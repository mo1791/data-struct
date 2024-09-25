// forward_list.hpp
#ifndef __FORWARD_LIST_HXX__
#define __FORWARD_LIST_HXX__

#pragma once

#include <ranges>
#include <utility>

namespace details
{
    // Concept to check if a type is neither the same as U nor derived from U
    template <class T, class U>
    concept non_self = std::conjunction<
        std::negation<std::is_same<std::decay_t<T>, U>>,
        std::negation<std::is_base_of<U, std::decay_t<T>>>>::value;
}

// Forward List Class Template
template <class Type>
struct forward_list final
{
private:
    // Nested Node and Iterator structs
    struct Node;
    struct Iterator;

private:
    using allocator_t = std::allocator<Node>;
    using traits      = std::allocator_traits<allocator_t>;

public:
    // Type Aliases
    using value_type = Type;

    using reference       = Type &;
    using const_reference = const Type &;

    using pointer       = Type *;
    using const_pointer = const Type *;

    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using iterator       = Iterator;
    using const_iterator = std::const_iterator<Iterator>;

    using iterator_category = std::forward_iterator_tag;
    using iterator_concept  = std::forward_iterator_tag;

public:
    // Constructors

    /**
    * @brief Default constructor.
    */
    constexpr forward_list() noexcept = default;

    /**
    * @brief Copy constructor.
    * @param outer The forward_list to copy.
    */
    constexpr forward_list(const forward_list &outer)
        : m_alloc{traits::select_on_copy_construction(outer.m_alloc)}
    {
        this->m_head = this->__M_clone(outer.m_head);
    }

    /**
    * @brief Move constructor.
    * @param outer The forward_list to move.
    */
    constexpr forward_list(forward_list &&outer) noexcept
        : m_alloc{std::move(outer.m_alloc)}
    {
        this->m_head = std::exchange(outer.m_head, nullptr);
    }

    /**
    * @brief Constructor from initializer list.
    * @param list The initializer list to initialize the forward_list.
    */
    constexpr forward_list(std::initializer_list<Type> list)
        : forward_list{std::ranges::begin(list), std::ranges::end(list)}
    {
    }

    /**
    * @brief Constructor from range.
    * @tparam range_t Type of the range.
    * @param range The range to initialize the forward_list.
    */
    template <typename range_t> requires std::ranges::range<range_t>
    constexpr forward_list(std::from_range_t, range_t &&range)
        requires(std::conjunction_v<
                 std::bool_constant<details::non_self<range_t, forward_list>>,
                 std::bool_constant<std::constructible_from<Type, std::ranges::range_value_t<range_t>>>>)
        : forward_list{std::ranges::begin(range), std::ranges::end(range)}
    {
    }

    /**
    * @brief Constructor from range defined by iterators.
    * @tparam Iterator Type of the iterator.
    * @tparam Sentinel Type of the sentinel.
    * @param first The beginning of the range.
    * @param last The end of the range.
    */
    template <typename Iterator, typename Sentinel>
        requires std::conjunction<
            std::bool_constant<std::input_iterator<Iterator>>,
            std::bool_constant<std::sentinel_for<Sentinel, Iterator>>
        >::value
    constexpr forward_list(Iterator first, Sentinel last)
        requires(std::constructible_from<Type, std::iter_value_t<Iterator>>)
        : forward_list{}
    {
        this->__M_range_init(first, last);
    }

public:
    // Assignment Operators

    /**
    * @brief Copy assignment operator.
    * @param outer The forward_list to copy.
    * @return forward_list& Reference to the current object.
    */
    constexpr auto operator=(const forward_list &outer)
            noexcept(std::disjunction<typename traits::propagate_on_container_copy_assignment,
                                        typename traits::is_always_equal>{})
        -> forward_list &
    {
        if (this == std::addressof(outer))
            return *this;

        this->__M_copy_assign(outer, typename traits::propagate_on_container_copy_assignment{});
        return *this;
    }

    /**
    * @brief Move assignment operator.
    * @param outer The forward_list to move.
    * @return forward_list& Reference to the current object.
    */
    constexpr auto operator=(forward_list &&outer)
            noexcept(std::disjunction<typename traits::propagate_on_container_copy_assignment,
                                        typename traits::is_always_equal>{})
        -> forward_list &
    {
        if (this == std::addressof(outer))
            return *this;

        constexpr bool is_propagate = std::disjunction<
            typename traits::propagate_on_container_move_assignment,
            typename traits::is_always_equal>{};

        this->__M_move_assign(outer, std::bool_constant<is_propagate>{});
        return *this;
    }

public:
    // Element Modifiers

    /**
    * @brief Adds an element to the front of the list.
    * @tparam UType Type of the element to be added.
    * @param data The element to be added.
    */
    template <class UType>
    constexpr void push_front(UType &&data)
        requires(std::constructible_from<Type, UType>)
    {
        this->m_head = this->__M_create_node(std::forward<UType>(data), this->m_head);
    }

    /**
    * @brief Emplaces an element to the front of the list.
    * @tparam ARGS Types of arguments for constructing the element.
    * @param args Arguments for constructing the element.
    */
    template <class... ARGS>
    constexpr void emplace_front(ARGS &&...args)
        requires(std::constructible_from<Type, ARGS...>)
    {
        this->m_head = this->__M_create_node(std::in_place, std::forward_as_tuple(args...),
                                             this->m_head);
    }

    /**
    * @brief Emplaces an element after a specified position.
    * @tparam _iterator Type of the iterator.
    * @tparam ARGS Types of arguments for constructing the element.
    * @param position Iterator pointing to the position after which the element should be emplaced.
    * @param args Arguments for constructing the element.
    */
    template <typename _iterator, class... ARGS>
        requires std::conjunction<
            std::bool_constant<std::input_iterator<_iterator>>,
            std::bool_constant<std::constructible_from<Type, ARGS...>>
        >::value
    constexpr void emplace_after(_iterator position, ARGS &&...args)
    {
        if (auto current = position.m_node)
        {
            current->m_next = this->__M_create_node(std::in_place, std::forward_as_tuple(args...),
                                                current->m_next);
        }
    }

    /**
    * @brief Inserts an element after a specified position.
    * @tparam _iterator Type of the iterator.
    * @tparam UType Type of the element to be inserted.
    * @param position Iterator pointing to the position after which the element should be inserted.
    * @param data The element to be inserted.
    */
    template <typename  _iterator, class UType>
        requires std::conjunction<
            std::bool_constant<std::input_iterator<_iterator>>,
            std::bool_constant<std::constructible_from<Type, UType>>
        >::value
    constexpr void insert_after(_iterator position, UType &&data)
    {
        if (auto current = position.m_node)
        {
            current->m_next = this->__M_create_node(std::forward<UType>(data), current->m_next);
        }
    }
    
    /**
    * @brief Inserts multiple copies of an element after a specified position.
    * @tparam _iterator Type of the iterator.
    * @tparam UType Type of the element to be inserted.
    * @param position Iterator pointing to the position after which the elements should be inserted.
    * @param count Number of elements to insert.
    * @param data The element to be inserted.
    */
    template <std::input_iterator _iterator, class UType>
        requires std::conjunction<
            std::bool_constant<std::input_iterator<_iterator>>,
            std::bool_constant<std::constructible_from<Type, UType>>
        >::value
    constexpr void insert_after(_iterator position, size_type count, UType &&data)
    {
        while (count--)
        {
            this->insert_after(position++, data);
        }
    }

    /**
    * @brief Inserts elements from a range after a specified position.
    * @tparam _iterator Type of the iterator.
    * @tparam Iterator Type of the iterator.
    * @tparam Sentinel Type of the sentinel.
    * @param position Iterator pointing to the position after which the elements should be inserted.
    * @param first Iterator pointing to the beginning of the range.
    * @param last Sentinel pointing to the end of the range.
    */
    template <typename _iterator, typename Iterator, typename Sentinel>
        requires std::conjunction<
            std::bool_constant<std::input_iterator<_iterator>>,
            std::bool_constant<std::input_iterator<Iterator>>,
            std::bool_constant<std::sentinel_for<Sentinel, Iterator>>,
            std::constructible_from<Type, std::iter_value_t<Iterator>>
        >::value
    constexpr void insert_after(_iterator position, Iterator first, Sentinel last)
    {
        while (first != last)
            this->insert_after(position++, *first++);
    }

public:
    // Element Access

    /**
    * @brief Removes the first element of the list.
    */
    constexpr void pop_front()
    {
        if (auto target = this->m_head)
        {
            this->m_head = this->m_head->m_next;

            traits::destroy(this->m_alloc, target);
            traits::deallocate(this->m_alloc, target, 1UL);
        }
    }

    /**
    * @brief Removes all elements from the list.
    */
    constexpr void clear()
    {
        while (this->m_head != nullptr)
            this->pop_front();
    }

    /**
    * @brief Erases the element after a specified position.
    * @tparam _iterator Type of the iterator.
    * @param prev Iterator pointing to the position before the element to be erased.
    */
    template <typename _iterator> requires std::input_iterator<_iterator>
    constexpr void erase_after(_iterator prev)
    {
        if (auto current = prev.m_node)
        {
            auto target = current->m_next;

            if (target == nullptr)
                return;

            current->m_next = target->m_next;

            traits::destroy(this->m_alloc, std::to_address(target));
            traits::deallocate(this->m_alloc,std::to_address(target), 1UL);
        }
    }

    /**
    * @brief Erases elements in the range after a specified position.
    * @tparam _iterator Type of the iterator.
    * @tparam _sentinel Type of the sentinel.
    * @param prev Iterator pointing to the position before the first element to be erased.
    * @param end Sentinel indicating the end of the range to be erased.
    */
    template <typename _iterator, typename _sentinel>
        requires std::conjunction<
            std::bool_constant<std::input_iterator<_iterator>>,
            std::bool_constant<std::sentinel_for<_sentinel, _iterator>>
        >::value
    constexpr void erase_after(_iterator prev, _sentinel end)
    {
    }

    /**
    * @brief Returns a reference to the first element of the list.
    * @return std::optional<std::reference_wrapper<Type>> Optional reference to the first element.
    */
    constexpr auto front() noexcept -> std::optional<std::reference_wrapper<Type>>
    {
        if (this->m_head == nullptr)
        {
            return std::nullopt;
        }
        return std::optional{std::ref(this->m_head->m_data)};
    }

    /**
    * @brief Returns a const reference to the first element of the list.
    * @return std::optional<const std::reference_wrapper<Type>> Optional const reference to the first element.
    */
    constexpr auto front() const noexcept -> std::optional<const std::reference_wrapper<Type>>
    {
        if (this->m_head == nullptr)
        {
            return std::nullopt;
        }
        return std::optional{std::cref(this->m_head->m_data)};
    }

public:
    // Capacity

    /**
    * @brief Checks if the list is empty.
    * @return bool True if the list is empty, false otherwise.
    */
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return (this->m_head == nullptr);
    }

    /**
    * @brief Returns the number of elements in the list.
    * @return size_type Number of elements in the list.
    */
    [[nodiscard]] constexpr auto size() const noexcept -> size_type
    {
        return std::ranges::distance(begin(), end());
    }

public:
    // Operations

    /**
    * @brief Reverses the order of elements in the list.
    */
    constexpr void reverse() noexcept
    {
        this->m_head = this->__M_reverse(this->m_head);
    }

public:
    // Iterators

    /**
    * @brief Returns an iterator to the beginning of the list.
    * @return iterator Iterator to the beginning of the list.
    */
    constexpr auto begin() noexcept -> iterator { return iterator{this->m_head}; }

    /**
    * @brief Returns a const iterator to the beginning of the list.
    * @return iterator Const iterator to the beginning of the list.
    */
    constexpr auto begin() const noexcept -> iterator { return iterator{this->m_head}; }

    /**
    * @brief Returns a const iterator to the beginning of the list.
    * @return const_iterator Const iterator to the beginning of the list.
    */
    constexpr auto cbegin() const noexcept -> const_iterator { return const_iterator{this->m_head}; }

    /**
    * @brief Returns an iterator to the end of the list.
    * @return iterator Iterator to the end of the list.
    */
    constexpr auto end() noexcept -> iterator { return iterator{}; }

    /**
    * @brief Returns a const iterator to the end of the list.
    * @return iterator Const iterator to the end of the list.
    */
    constexpr auto end() const noexcept -> iterator { return iterator{}; }

    /**
    * @brief Returns a const iterator to the end of the list.
    * @return const_iterator Const iterator to the end of the list.
    */
    constexpr auto cend() const noexcept -> const_iterator { return const_iterator{}; }

public:
    // Destructor

    /**
    * @brief Destructor. Destroys all elements of the list.
    */
    constexpr ~forward_list() { this->clear(); }

public:
    // Friend Function

    /**
    * @brief Swaps the contents of two lists.
    * @param lhs First list to swap.
    * @param rhs Second list to swap.
    */
    friend constexpr void swap(forward_list &lhs, forward_list &rhs)
            noexcept(std::disjunction<typename traits::propagate_on_container_swap,
                                        typename traits::is_always_equal>{})
    {
        if constexpr (typename traits::propagate_on_container_swap{})
        {
            std::swap(lhs.m_alloc, rhs.m_alloc);
        }

        std::swap(lhs.m_head, rhs.m_head);
    }

private:
    // Private Member Functions

    /**
    * @brief Reverses the order of nodes in the list.
    * @param head Pointer to the head of the list.
    * @return Node* Pointer to the new head of the reversed list.
    */
    constexpr auto __M_reverse(Node *head) noexcept -> Node *
    {
        Node *prev = nullptr;
        Node *curr = head;

        while (curr != nullptr)
        {
            auto next = curr->m_next;

            curr->m_next = prev;
            prev = curr;
            curr = next;
        }
        return prev;
    }

    /**
    * @brief Creates a new node with the given arguments.
    * @tparam ARGS Types of arguments for constructing the node.
    * @param args Arguments for constructing the node.
    * @return Node* Pointer to the newly created node.
    */
    template <class... ARGS>
    constexpr Node *__M_create_node(ARGS &&...args)
    {
        auto node = traits::allocate(this->m_alloc, 1UL);

        try {
            traits::construct(this->m_alloc, std::to_address(node), std::forward<ARGS>(args)...);
        }
        catch (...) {
            traits::deallocate(this->m_alloc, std::to_address(node), 1UL);
            throw;
        }
        return node;
    }

    /**
    * @brief Initializes the list from a range defined by iterators.
    * @tparam _iterator Type of the iterator.
    * @tparam _sentinel Type of the sentinel.
    * @param first Iterator pointing to the beginning of the range.
    * @param last Sentinel indicating the end of the range.
    */
    template <typename _iterator, typename _sentinel>
        requires std::conjunction<
            std::bool_constant<std::input_iterator<_iterator>>,
            std::bool_constant<std::sentinel_for<_sentinel, _iterator>>
        >::value
    constexpr void __M_range_init(_iterator first, _sentinel last)
    {
        if (first == last)
            return;

        this->m_head = this->__M_create_node(*first);

        auto cursor = this->m_head;

        first = std::ranges::next(first);

        while (first != last)
        {
            cursor->m_next = this->__M_create_node(*first);

            cursor = cursor->m_next;
            first = std::ranges::next(first);
        }
    }

    /**
    * @brief Clones the nodes of another list.
    * @param source Pointer to the head of the source list.
    * @return Node* Pointer to the head of the cloned list.
    */
    constexpr Node *__M_clone(Node *source)
    {
        if (source == nullptr)
        {
            this->clear();
            return source;
        }

        auto node = this->__M_create_node(source->m_data);
        auto copy = node;
        auto cursor = copy;

        source = source->m_next;

        while (source != nullptr)
        {
            node = this->__M_create_node(source->m_data);

            cursor->m_next = node;

            cursor = cursor->m_next;
            source = source->m_next;
        }

        return copy;
    }

    /**
    * @brief Assigns elements from a range to the list.
    * @tparam _iterator Type of the iterator.
    * @tparam _sentinel Type of the sentinel.
    * @param first Iterator pointing to the beginning of the range.
    * @param last Sentinel indicating the end of the range.
    */
    template <typename _iterator, typename _sentinel>
        requires std::conjunction<
            std::bool_constant<std::input_iterator<_iterator>>,
            std::bool_constant<std::sentinel_for<_sentinel, _iterator>>
        >::value
    constexpr void __M_assign(_iterator first, _sentinel last)
    {
        if (first == last)
        {
            this->clear();
            return;
        }

        if (this->m_head == nullptr)
        {
            this->__M_range_init(first, last);
            return;
        }

        auto begin = std::ranges::begin(*this);
        auto end = std::ranges::end(*this);

        iterator prev{};

        while ((first != last) && (begin != end))
        {
            *begin = *first;

            prev = begin;
            begin = std::ranges::next(begin);
            first = std::ranges::next(first);
        }

        if (first != last)
        {
            this->insert_after(prev, first, last);
        }
        else
        {
            while (prev != end)
            {
                this->erase_after(prev);
            }
        }
    }

    /**
    * @brief Assigns elements from another list to the current list.
    * @param outer The forward_list to copy.
    * @param std::true_type Indicates copy assignment.
    */
    constexpr void __M_copy_assign(const forward_list &outer, std::true_type)
    {
        if constexpr (not typename traits::is_always_equal{})
        {
            if (this->m_alloc != outer.m_alloc)
            {
                this->clear();
            }
            this->m_alloc = outer.m_alloc;
        }

        this->__M_assign(std::ranges::begin(outer), std::ranges::end(outer));
    }

    constexpr void __M_copy_assign(const forward_list &outer, std::false_type)
    {
        this->clear();
        this->__M_assign(std::ranges::begin(outer), std::ranges::end(outer));
    }

    /**
    * @brief Assigns elements from another list to the current list.
    * @param outer The forward_list to move.
    * @param std::false_type Indicates move assignment.
    */
    constexpr void __M_move_assign(forward_list &outer, std::true_type)
    {
        this->clear();

        this->m_alloc = std::move(outer.m_alloc);
        this->m_head = std::exchange(outer.m_head, nullptr);
    }

    constexpr void __M_move_assign(forward_list &outer, std::false_type)
    {
        if (this->m_alloc == outer.m_alloc)
        {
            this->__M_move_assign(outer, std::true_type{});
        }
        else
        {
            this->m_head = std::exchange(outer.m_head, nullptr);
        }
    }

private:
    // Private Members
    Node *m_head{nullptr};
    [[no_unique_address]] allocator_t m_alloc{};
};



// Deduction guides

/**
* @brief Deduction guide for forward_list from range.
* @tparam range_t Type of the range.
*/
template <class range_t>
forward_list(std::from_range_t, range_t &&) -> forward_list<std::ranges::range_value_t<range_t>>;

/**
* @brief Deduction guide for forward_list from iterators and sentinels.
* @tparam Iterator Type of the iterator.
* @tparam Sentinel Type of the sentinel.
*/
template <class Iterator, class Sentinel>
forward_list(Iterator, Sentinel) -> forward_list<std::iter_value_t<Iterator>>;



// Node Struct
template <class Type>
struct forward_list<Type>::Node final
{
public:
    // Public Member Functions

    /**
    * @brief Default constructor.
    */
    constexpr Node() noexcept(std::is_nothrow_default_constructible_v<Type>)
        requires(std::default_initializable<Type>)
    = default;

    /**
    * @brief Constructor with data.
    * @param data The data of the node.
    * @param next Pointer to the next node.
    */
    constexpr Node(const Type &data, Node *next = nullptr) noexcept(std::is_nothrow_copy_constructible_v<Type>)
        requires(std::copy_constructible<Type>)
        : m_data{data}, m_next{next}
    {
    }

    /**
    * @brief Constructor with data.
    * @param data The data of the node.
    * @param next Pointer to the next node.
    */
    constexpr Node(Type &&data, Node *next = nullptr) noexcept(std::is_nothrow_move_constructible_v<Type>)
        requires(std::move_constructible<Type>)
        : m_data{std::move(data)}, m_next{next}
    {
    }

    /**
    * @brief Constructor with tuple.
    * @tparam Tuple Type of the tuple.
    * @tparam ARGS Types of arguments for constructing the node.
    * @param tuple The tuple with arguments for constructing the node.
    * @param next Pointer to the next node.
    */
    template <template <class...> class Tuple, class... ARGS>
    constexpr Node(std::in_place_t, const Tuple<ARGS...> &tuple, Node *next = nullptr)
        noexcept(std::is_nothrow_constructible_v<Type, ARGS...>)
        requires std::constructible_from<Type, ARGS...>
        : m_data{std::make_from_tuple<Type>(tuple)}, m_next{next}
    {
    }

private:
    Type m_data{};
    Node *m_next{nullptr};
};


// Iterator Struct
template <class Type>
struct forward_list<Type>::Iterator final
{
public:
    using value_type = Type;

    using reference       = Type&;
    using const_reference = const Type&;

    using pointer       = Type*;
    using const_pointer = const Type*

    using difference_type = std::ptrdiff_t;

    using iterator_category = std::forward_iterator_tag;
    using iterator_concept  = std::forward_iterator_tag;
    
public:
    // Public Member Functions

    /**
    * @brief Default constructor.
    */
    constexpr Iterator() noexcept = default;

    /**
    * @brief Constructor with node.
    * @param node Pointer to the node.
    */
    constexpr Iterator(Node *node) noexcept : m_node{node} {}

    /**
    * @brief Dereference operator.
    * @return reference Reference to the data of the node.
    */
    constexpr auto operator*() noexcept -> reference { return this->m_node->m_data; }

    /**
    * @brief Dereference operator.
    * @return reference Const reference to the data of the node.
    */
    constexpr auto operator*() const noexcept -> reference { return this->m_node->m_data }

    /**
    * @brief Arrow operator.
    * @return pointer Pointer to the data of the node.
    */
    constexpr auto operator->() noexcept -> pointer { return std::addressof(this->m_node->m_data); }

    /**
    * @brief Arrow operator.
    * @return pointer Const pointer to the data of the node.
    */
    constexpr auto operator->() const noexcept -> pointer { return std::addressof(this->m_node->m_data); }

    /**
    * @brief Pre-increment operator.
    * @return Iterator& Reference to the incremented iterator.
    */
    constexpr auto operator++() noexcept -> Iterator &
    {
        this->m_node = this->m_node->m_next;
        return *this;
    }

    /**
    * @brief Post-increment operator.
    * @return Iterator Copy of the iterator before incrementing.
    */
    constexpr auto operator++(int) noexcept -> Iterator
    {
        auto copy = *this;
        std::ranges::advance(*this, 1UL);

        return copy;
    }

    /**
    * @brief Equality operator.
    * @param lhs Left-hand side iterator.
    * @param rhs Right-hand side iterator.
    * @return bool True if iterators are equal, false otherwise.
    */
    friend constexpr auto operator==(const Iterator &lhs,
                                     const Iterator &rhs) noexcept -> bool
    {
        return (lhs.m_node == rhs.m_node);
    }

private:
    // Private Member
    Node *m_node{nullptr};
};


#endif
// Definition of the rest of the functions...
