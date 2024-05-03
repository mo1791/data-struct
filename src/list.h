#ifndef __LIST_HXX__
#define __LIST_HXX__


#include <cassert>
#include <ranges>
#include <utility>

namespace details
{
    /**
    * @brief Concept ensuring two types are not the same or related through inheritance.
    */
    template <class T, class U>
    concept non_self = std::conjunction_v<
        std::negation<std::bool_constant<std::same_as<std::decay_t<T>, U>>>,
        std::negation<std::bool_constant<std::derived_from<std::decay_t<T>, U>>>>;
} // namespace details

/**
* @brief A doubly linked list implementation.
* @tparam Type The type of elements stored in the list.
*/
template <typename Type>
struct list final
{

private:
    struct Node;
    struct Iterator;

private:
    using allocator_t = std::allocator<Node>;
    using traits      = std::allocator_traits<allocator_t>;

public:
    // Member type aliases

    using value_type = Type;

    using reference       = Type &;
    using const_reference = const Type &;

    using pointer       = Type *;
    using const_pointer = const Type *;

    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using iterator       = Iterator;
    using const_iterator = std::const_iterator<iterator>;

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using iterator_category = std::bidirectional_iterator_tag;
    using iterator_concept  = std::bidirectional_iterator_tag;

public:
    // Constructors and Destructor
    /**
    * @brief Constructs an empty list.
    * @details Requires that the stored type is default initializable.
    */
    constexpr list()
        requires std::default_initializable<Type>
    {
        this->m_head = this->__M_create_node();
    }

    /**
    * @brief Constructs the list with elements from a range.
    * @param first The beginning of the range.
    * @param last The end of the range.
    * @tparam Iterator The type of iterator representing the range.
    * @tparam Sentinel The type of sentinel indicating the end of the range.
    * @details Requires that the stored type is constructible from the value type of the iterator.
    */
    template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
    explicit constexpr list(Iterator first, Sentinel last)
        requires std::constructible_from<Type, std::iter_value_t<Iterator>>
        : list{}
    {
        this->__M_range_init(first, last);
    }

    /**
    * @brief Constructs the list with elements from an initializer list.
    * @param lst The initializer list.
    */
    constexpr list(std::initializer_list<Type> lst)
        : list(std::ranges::begin(lst), std::ranges::end(lst))
    {
    }

    /**
    * @brief Constructs the list with elements from a range with concept constraints.
    * @param from_range_t Tag type to disambiguate from the iterator constructor.
    * @param range The range to initialize the list.
    * @tparam range_t The type of range.
    * @details Requires that the range type is not related to the list and the stored type is constructible from the value type of the range.
    */
    template <std::ranges::range range_t>
    constexpr list(std::from_range_t, range_t &&range)
        requires(std::conjunction<
                 std::bool_constant<details::non_self<range_t, list>>,
                 std::bool_constant<std::constructible_from<Type, std::ranges::range_value_t<range_t>>>>::value)
        : list(std::ranges::begin(range), std::ranges::end(range))
    {
    }

    /**
    * @brief Copy constructor.
    * @param outer The list to copy.
    * @details Uses the allocator to copy nodes from the source list.
    */
    constexpr list(const list &outer) noexcept(std::is_nothrow_copy_constructible<allocator_t>{})
        : m_alloc{traits::select_on_container_copy_construction(outer.m_alloc)}
    {
        this->m_head = this->__M_clone(outer.m_head->m_next);
    }

    /**
    * @brief Move constructor.
    * @param outer The list to move.
    * @details Moves the allocator and head pointer from the source list.
    */
    constexpr list(list &&outer) noexcept(std::is_nothrow_move_constructible<allocator_t>{})
        : m_alloc{std::move(outer.m_alloc)}
    {
        this->m_head = std::exchange(outer.m_head, this->__M_create_node());
    }

    /**
    * @brief Copy assignment operator.
    * @param rhs The list to copy.
    * @return Reference to this list.
    * @details Performs a deep copy of the source list.
    */
    constexpr list &operator=(const list &rhs) noexcept(std::disjunction<
                                                        typename traits::propagate_on_container_copy_assignment,
                                                        typename traits::is_always_equal>{})
    {
        if (this == std::addressof(rhs))
        {
            return *this;
        }

        this->__M_copy_assign(rhs, typename traits::propagate_on_container_copy_assignment{});
        return *this;
    }

    /**
    * @brief Move assignment operator.
    * @param rhs The list to move.
    * @return Reference to this list.
    * @details Moves the content from the source list to this list.
    */
    constexpr list &operator=(list &&rhs) noexcept(std::disjunction<
                                                   typename traits::propagate_on_container_copy_assignment,
                                                   typename traits::is_always_equal>{})
    {
        if (this == std::addressof(rhs))
            return *this;

        constexpr bool _is_propagate =
            std::disjunction<typename traits::propagate_on_container_move_assignment,
                             typename traits::is_always_equal>{};

        this->__M_move_assign(rhs, std::bool_constant<_is_propagate>{});
        return *this;
    }

    /**
    * @brief Destructor.
    * @details Clears the list and deallocates memory.
    */
    constexpr ~list() { this->clear(); }

public:
    // Element Insertion
    // (methods for pushing, inserting, and emplacing elements)

    /**
    * @brief Adds an element to the end of the list.
    * @param data The element to be added.
    * @tparam UType The type of the element.
    */
    template <class UType>
    constexpr void push_back(UType &&data)
    {
        assert(this->m_head != nullptr);

        this->m_head->push_back(this->__M_create_node(std::forward<UType>(data)));
    }

    /**
    * @brief Adds an element to the front of the list.
    * @param data The element to be added.
    * @tparam UType The type of the element.
    */
    template <class UType>
    constexpr void push_front(UType &&data)
    {
        assert(this->m_head != nullptr);

        this->m_head->push_front(this->__M_create_node(std::forward<UType>(data)));
    }

    /**
    * @brief Inserts an element after a specified position.
    * @param position The iterator specifying the position to insert after.
    * @param data The element to be inserted.
    * @tparam _iterator The type of iterator.
    * @tparam UType The type of the element.
    */
    template <std::input_iterator _iterator, class UType>
    constexpr void insert_after(_iterator position, UType &&data)
    {
        Node *node = position.m_cursor;
        node->push_front(this->__M_create_node(std::forward<UType>(data)));
    }

    /**
    * @brief Inserts multiple copies of an element after a specified position.
    * @param position The iterator specifying the position to insert after.
    * @param count The number of copies to insert.
    * @param data The element to be inserted.
    * @tparam _iterator The type of iterator.
    * @tparam UType The type of the element.
    */
    template <std::input_iterator _iterator, class UType>
    constexpr void insert_after(_iterator position, size_type count, UType &&data)
    {
        while (count--)
            this->insert_after(position, std::forward<UType>(data));
    }

    /**
    * @brief Inserts elements from a range after a specified position.
    * @param position The iterator specifying the position to insert after.
    * @param first The beginning of the range.
    * @param last The end of the range.
    * @tparam _iterator The type of iterator.
    * @tparam _Iterator The type of iterator representing the range.
    * @tparam _Sentinel The type of sentinel indicating the end of the range.
    */
    template <std::input_iterator _iterator, std::input_iterator _Iterator,
              std::sentinel_for<_Iterator> _Sentinel>
    constexpr void insert_after(_iterator position, _Iterator first, _Sentinel last)
    {
        while (first != last)
        {
            this->insert_after(position, *first);
            position = std::ranges::next(position);
            first = std::ranges::next(first);
        }
    }

    /**
    * @brief Inserts an element before a specified position.
    * @param position The iterator specifying the position to insert before.
    * @param data The element to be inserted.
    * @tparam _iterator The type of iterator.
    * @tparam UType The type of the element.
    */
    template <std::input_iterator _iterator, class UType>
    constexpr void insert_before(_iterator position, UType &&data)
    {
        Node *node = position.m_cursor;
        node->push_back(this->__M_create_node(std::forward<UType>(data)));
    }

    /**
    * @brief Inserts multiple copies of an element before a specified position.
    * @param position The iterator specifying the position to insert before.
    * @param count The number of copies to insert.
    * @param data The element to be inserted.
    * @tparam _iterator The type of iterator.
    * @tparam UType The type of the element.
    */
    template <std::input_iterator _iterator, class UType>
    constexpr void insert_before(_iterator position, size_type count, UType &&data)
    {
        while (count--)
            this->insert_before(position, std::forward<UType>(data));
    }

    /**
    * @brief Inserts elements from a range before a specified position.
    * @param position The iterator specifying the position to insert before.
    * @param first The beginning of the range.
    * @param last The end of the range.
    * @tparam _iterator The type of iterator.
    * @tparam _Iterator The type of iterator representing the range.
    * @tparam _Sentinel The type of sentinel indicating the end of the range.
    */
    template <std::input_iterator _iterator, std::input_iterator _Iterator,
              std::sentinel_for<_Iterator> _Sentinel>
    constexpr void insert_before(_iterator position, _Iterator first, _Sentinel last)
    {
        while (first != last)
            this->insert_before(position, *first++);
    }

    /**
    * @brief Emplaces an element to the end of the list.
    * @tparam ARGS The types of arguments for constructing the element.
    */
    template <class... ARGS>
    constexpr void emplace_back(ARGS &&...args)
    {
        assert(this->m_head != nullptr);

        this->m_head->push_back(this->__M_create_node(std::in_place, std::forward<ARGS>(args)...));
    }

    /**
    * @brief Emplaces an element to the front of the list.
    * @tparam ARGS The types of arguments for constructing the element.
    */
    template <class... ARGS>
    constexpr void emplace_front(ARGS &&...args)
    {
        assert(this->m_head != nullptr);

        this->m_head->push_front(this->__M_create_node(std::in_place, std::forward<ARGS>(args)...));
    }

    /**
    * @brief Emplaces an element after a specified position.
    * @tparam ARGS The types of arguments for constructing the element.
    */
    template <std::input_iterator _iterator, class... ARGS>
    constexpr void emplace_after(_iterator position, ARGS &&...args)
    {
        assert(this->m_head != nullptr);

        Node *node = position.m_cursor;

        node->push_front(this->__M_create_node(std::in_place, std::forward<ARGS>(args)...));
    }

    /**
    * @brief Emplaces an element before a specified position.
    * @tparam ARGS The types of arguments for constructing the element.
    */
    template <std::input_iterator _iterator, class... ARGS>
    constexpr void emplace_before(_iterator position, ARGS &&...args)
    {
        assert(this->m_head != nullptr);

        Node *node = position.m_cursor;

        node->push_back(this->__M_create_node(std::in_place, std::forward<ARGS>(args)...));
    }

public:
    // Element Removal
    // (methods for popping elements, erasing elements, and clearing the list)

    /**
    * @brief Removes the first element from the list.
    */
    constexpr void pop_front()
    {
        this->erase(iterator{this->m_head->m_next});
    }

    /**
    * @brief Removes the last element from the list.
    */
    constexpr void pop_back()
    {
        this->erase(iterator{this->m_head->m_prev});
    }

    /**
    * @brief Erases the element at the specified position.
    * @param position Iterator pointing to the element to be erased.
    * @tparam _iterator Type of the iterator.
    * @return Iterator pointing to the element following the erased element.
    */
    template <std::input_iterator _iterator>
    constexpr iterator erase(_iterator position)
    {
        auto target = position.m_cursor;

        target->m_prev->m_next = target->m_next;
        target->m_next->m_prev = target->m_prev;

        auto next = target->m_next;

        traits::destroy(this->m_alloc, target);
        traits::deallocate(this->m_alloc, target, 1UL);

        target = nullptr;

        return next;
    }

    /**
    * @brief Erases elements in the range [first, last).
    * @param first Iterator pointing to the first element to be erased.
    * @param last Iterator pointing to the element following the last element to be erased.
    * @tparam _iterator Type of the iterator.
    * @tparam _sentinel Type of the sentinel.
    * @return Iterator pointing to the element following the last erased element.
    */
    template <std::input_iterator _iterator, std::sentinel_for<_iterator> _sentinel>
    constexpr iterator erase(_iterator first, _sentinel last)
    {
        while (first != last)
            this->erase(first++);

        return last;
    }

    /**
    * @brief Clears the entire list, deallocating all nodes.
    */
    constexpr void clear()
    {
        while (not this->empty())
            this->pop_front();

        traits::destroy(this->m_alloc, this->m_head);
        traits::deallocate(this->m_alloc, this->m_head, 1UL);

        this->m_head = nullptr;
    }

public:
    // Element Access
    // (methods for accessing the first and last elements of the list)

    /**
    * @brief Returns the first element of the list, if any.
    * @return An optional reference to the first element of the list.
    */
    constexpr auto front() noexcept -> std::optional<std::reference_wrapper<Type>>
    {
        if (this->empty())
        {
            return std::nullopt;
        }
        return std::optional{std::ref(this->m_head->m_next->m_data)};
    }

    /**
    * @brief Returns the first element of the list (const version), if any.
    * @return An optional constant reference to the first element of the list.
    */
    constexpr auto front() const noexcept -> std::optional<const std::reference_wrapper<Type>>
    {
        if (this->empty())
        {
            return std::nullopt;
        }
        return std::optional{std::cref(this->m_head->m_next->m_data)};
    }

    /**
    * @brief Returns the last element of the list, if any.
    * @return An optional reference to the last element of the list.
    */
    constexpr auto back() noexcept -> std::optional<std::reference_wrapper<Type>>
    {
        if (this->empty())
        {
            return std::nullopt;
        }
        return std::optional{std::ref(this->m_head->m_prev->m_data)};
    }

    /**
    * @brief Returns the last element of the list (const version), if any.
    * @return An optional constant reference to the last element of the list.
    */
    constexpr auto back() const noexcept -> std::optional<const std::reference_wrapper<Type>>
    {
        if (this->empty())
        {
            return std::nullopt;
        }
        return std::optional{std::cref(this->m_head->m_prev->m_data)};
    }

public:
    // Size Querying and Emptiness Check
    // (methods for determining the size and emptiness of the list)

    /**
    * @brief Returns the number of elements in the list.
    * @return The number of elements in the list.
    */
    [[nodiscard]] constexpr auto size() const noexcept -> size_type
    {
        return std::distance(this->begin(), this->end());
    }

    /**
    * @brief Checks if the list is empty.
    * @return True if the list is empty, false otherwise.
    */
    constexpr auto empty() const noexcept -> bool
    {
        return (this->m_head != nullptr) || ((this->m_head->m_prev == this->m_head) && (this->m_head->m_next == this->m_head));
    }

public:
    // Iterator Support
    // (methods for obtaining iterators to the beginning and end of the list)

    /**
    * @brief Returns an iterator to the beginning of the list.
    * @return An iterator to the beginning of the list.
    */
    constexpr auto begin() noexcept -> iterator { return iterator{this->m_head->m_next}; }

    /**
    * @brief Returns a constant iterator to the beginning of the list.
    * @return A constant iterator to the beginning of the list.
    */
    constexpr auto begin() const noexcept -> iterator { return iterator{this->m_head->m_next}; }

    /**
    * @brief Returns a constant iterator to the beginning of the list.
    * @return A constant iterator to the beginning of the list.
    */
    constexpr auto cbegin() const noexcept -> const_iterator { return const_iterator{this->m_head->m_next}; }

    /**
    * @brief Returns a reverse iterator to the beginning of the list.
    * @return A reverse iterator to the beginning of the list.
    */
    constexpr auto rbegin() noexcept -> reverse_iterator { return reverse_iterator{this->m_head}; }

    /**
    * @brief Returns a reverse iterator to the beginning of the list (const version).
    * @return A reverse iterator to the beginning of the list (const version).
    */
    constexpr auto rbegin() const noexcept -> reverse_iterator { return reverse_iterator{this->m_head}; }

    /**
    * @brief Returns a constant reverse iterator to the beginning of the list.
    * @return A constant reverse iterator to the beginning of the list.
    */
    constexpr auto crbegin() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator{this->m_head};
    }

    /**
    * @brief Returns an iterator to the end of the list.
    * @return An iterator to the end of the list.
    */
    constexpr auto end() noexcept -> iterator { return iterator{this->m_head}; }

    /**
    * @brief Returns an iterator to the end of the list (const version).
    * @return An iterator to the end of the list (const version).
    */
    constexpr auto end() const noexcept -> iterator { return iterator{this->m_head}; }

    /**
    * @brief Returns a constant iterator to the end of the list.
    * @return A constant iterator to the end of the list.
    */
    constexpr auto cend() const noexcept -> const_iterator { return const_iterator{this->m_head}; }

    /**
    * @brief Returns a reverse iterator to the end of the list.
    * @return A reverse iterator to the end of the list.
    */
    constexpr auto rend() noexcept -> reverse_iterator
    {
        return reverse_iterator{this->m_head->m_next};
    }

    /**
    * @brief Returns a reverse iterator to the end of the list (const version).
    * @return A reverse iterator to the end of the list (const version).
    */
    constexpr auto rend() const noexcept -> reverse_iterator
    {
        return reverse_iterator{this->m_head->m_next};
    }

    /**
    * @brief Returns a constant reverse iterator to the end of the list.
    * @return A constant reverse iterator to the end of the list.
    */
    constexpr auto crend() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator{this->m_head->m_next};
    }

public:
    // Friend Function

    // Swapping Lists
    // (function for swapping the contents of two lists)

    /**
    * @brief Swaps the contents of two lists.
    *
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
    // (helper functions for memory management and list operations)

    /**
    * @brief Helper function to create a new node.
    * @tparam ARGS Types of arguments for constructing the node.
    * @param args Arguments forwarded to the node constructor.
    * @return Pointer to the newly created node.
    */
    template <class... ARGS>
    constexpr Node *__M_create_node(ARGS &&...args)
    {
        Node *node = traits::allocate(this->m_alloc, 1UL);

        try {
            traits::construct(this->m_alloc, node, std::forward<ARGS>(args)...);
        }
        catch (...) {
            traits::deallocate(this->m_alloc, node, 1UL);
            throw;
        }
        return node;
    }

    /**
    * @brief Helper function to clone nodes from another list.
    * @param source Pointer to the node from which to start cloning.
    * @return Pointer to the cloned list.
    */
    constexpr Node *__M_clone(Node *source)
    {
        if (source == nullptr)
        {
            this->clear();
            return source;
        }

        auto cursor = this->__M_create_node();

        auto end = source->m_prev;

        while (source != end)
        {
            cursor->push_back(this->__M_create_node(source->m_data));
            source = source->m_next;
        }
        return cursor;
    }

    /**
    * @brief Helper function to initialize the list with elements from a range.
    * @tparam _iterator Type of the iterator.
    * @tparam _sentinel Type of the sentinel.
    * @param first Iterator pointing to the first element of the range.
    * @param last Sentinel indicating the end of the range.
    */
    template <std::input_iterator _iterator, std::sentinel_for<_iterator> _sentinel>
    constexpr void __M_range_init(_iterator first, _sentinel last)
    {
        for (; first != last; first = std::ranges::next(first))
        {
            this->m_head->push_back(this->__M_create_node(*first));
        }
    }

    /**
    * @brief Helper function to assign elements from a range.
    * @tparam _iterator Type of the iterator.
    * @tparam _sentinel Type of the sentinel.
    * @param first Iterator pointing to the first element of the range.
    * @param last Sentinel indicating the end of the range.
    */
    template <std::input_iterator _iterator, std::sentinel_for<_iterator> _sentinel>
    constexpr void __M_assign(_iterator first, _sentinel last)
    {
        if (first == last)
        {
            this->clear();
            return;
        }

        if (this->empty())
        {
            this->__M_range_init(first, last);
            return;
        }

        auto begin = std::ranges::begin(*this);
        auto end = std::ranges::end(*this);

        while ((first != last) && (begin != end))
        {
            *begin = *first;
            begin = std::ranges::next(begin);
            first = std::ranges::next(first);
        }

        if (first != last) {
            this->insert_before(begin, first, last);
        }
        else {
            this->erase(begin, end);
        }
    }

    /**
    * @brief Helper function for copy assignment.
    * @param outer Reference to the list to copy from.
    * @param propagate_on_copy Assignment policy indicating whether allocator should be propagated.
    */
    constexpr void __M_copy_assign(const list &outer, std::true_type)
    {
        if constexpr (not typename traits::is_always_equal{})
        {
            if (this->m_alloc != outer.m_alloc) {
                this->clear();
            }
            this->m_alloc = outer.m_alloc;
        }

        this->__M_assign(std::ranges::begin(outer), std::ranges::end(outer));
    }

    /**
    * @brief Helper function for copy assignment with propagation.
    * @param outer Reference to the list to copy from.
    * @param propagate_on_copy Assignment policy indicating whether allocator should be propagated.
    */
    constexpr void __M_copy_assign(const list &outer, std::false_type)
    {
        this->clear();
        this->__M_assign(std::ranges::begin(outer), std::ranges::end(outer));
    }

    /**
    * @brief Helper function for move assignment.
    * @param outer Reference to the list to move from.
    * @param propagate_on_move Assignment policy indicating whether allocator should be propagated.
    */
    constexpr void __M_move_assign(list &outer, std::true_type)
    {
        this->clear();

        this->m_alloc = std::move(outer.m_alloc);
        this->m_head  = std::exchange(outer.m_head, this->__M_create_node());
    }

    /**
    * @brief Helper function for move assignment with propagation.
    * @param outer Reference to the list to move from.
    * @param propagate_on_move Assignment policy indicating whether allocator should be propagated.
    */
    constexpr void __M_move_assign(list &outer, std::false_type)
    {
        if (this->m_alloc == outer.m_alloc) {

            this->__M_move_assign(outer, std::true_type{});
        }
        else {
            this->m_head = std::exchange(outer.m_head, this->__M_create_node());
        }
    }

private:
    Node *m_head;                              // Pointer to the head of the list
    [[no_unique_address]] allocator_t m_alloc; // Allocator
};

// Deduction guides

/**
* @brief Deduction guide for list from range.
* @tparam range_t Type of the range.
*/
template <typename range_t>
list(std::from_range_t, range_t &&) -> list<std::ranges::range_value_t<range_t>>;

/**
* @brief Deduction guide for list from iterators and sentinels.
* @tparam Iterator Type of the iterator.
* @tparam Sentinel Type of the sentinel.
*/
template <typename iterator, typename sentinel>
list(iterator, sentinel) -> list<std::iter_value_t<iterator>>;



/**
* This class template represents a node in a linked list, containing a piece of data
* of type Type and pointers to the previous and next nodes in the list.
* @brief Represents an individual node in a linked list.
* @tparam Type The type of data held by the node.
*/
template <typename Type>
struct list<Type>::Node final
{
public:
    // Grant access to the list class
    friend struct list;

    // Grant access to the Iterator struct
    friend struct Iterator;

public:
    /**
    * @brief Default constructor.
    * Constructs a node with default-initialized data.
    */
    constexpr Node() noexcept(std::is_nothrow_default_constructible_v<Type>)
        requires std::default_initializable<Type>
    = default;

    /**
    * @brief Constructs a node with the given data.
    * @param data The data to be stored in the node.
    */
    explicit constexpr Node(Type const &data) noexcept(std::is_nothrow_copy_constructible_v<Type>)
        requires std::copy_constructible<Type>
        : m_data{data}
    {
    }

    /**
    * @brief Constructs a node by moving the given data.
    * @param data The data to be moved and stored in the node.
    */
    explicit constexpr Node(Type &&data) noexcept(std::is_nothrow_move_constructible_v<Type>)
        requires std::move_constructible<Type>
        : m_data{std::move(data)}
    {
    }

    /**
    * Constructs a node in place using perfect forwarding and the provided arguments.
    * @brief Constructs a node in place with the given arguments.
    * @tparam ARGS The types of arguments used for constructing the data.
    * @param args The arguments forwarded to construct the data in place.
    */
    template <class... ARGS>
    constexpr Node(std::in_place_t, ARGS &&...args) noexcept(std::is_nothrow_constructible_v<Type, ARGS...>)
        requires std::is_constructible_v<Type, ARGS...>
        : m_data{std::forward<ARGS>(args)...}
    {
    }

private:
    /**
    * This function inserts the given node in front of the current node in the linked list.
    * @brief Inserts a node in front of the current node.
    * @param node Pointer to the node to be inserted.
    */
    constexpr void push_front(Node *node) noexcept
    {
        node->m_next = this->m_next;
        node->m_prev = this;
        this->m_next->m_prev = node;
        this->m_next = node;
    }

    /**
    * This function inserts the given node behind the current node in the linked list.
    * @brief Inserts a node behind the current node.
    * @param node Pointer to the node to be inserted.
    */
    constexpr void push_back(Node *node) noexcept
    {
        node->m_prev = this->m_prev;
        node->m_next = this;
        this->m_prev->m_next = node;
        this->m_prev = node;
    }

private:
    Type m_data{};      // Data stored in the node
    Node *m_prev{this}; // Pointer to the previous node
    Node *m_next{this}; // Pointer to the next node
};

/**
* This class template represents an iterator for traversing elements of a linked list.
* It provides bidirectional traversal capabilities.
* @brief Represents an iterator for traversing a linked list.
* @tparam Type The type of data stored in the linked list.
*/
template <class Type>
struct list<Type>::Iterator final
{
public:
    friend struct list;

public:
    using value_type = Type;

    using reference       = Type &;                                  
    using const_reference = const Type &;

    using pointer       = Type *;                                 
    using const_pointer = const Type *; 

    using difference_type = std::ptrdiff_t;

    using iterator_category = std::bidirectional_iterator_tag; 
    using iterator_concept  = std::bidirectional_iterator_tag;  

public:
    // Constructors
    /**
    * Constructs an iterator pointing to nullptr.
    * @brief Default constructor.
    */
    constexpr Iterator() noexcept = default;

    /**
    * Constructs an iterator pointing to the provided node.
    * @brief Constructs an iterator with a given node pointer.
    * @param cursor Pointer to the node.
    */
    constexpr Iterator(Node *cursor) noexcept
        : m_cursor{cursor}
    {
    }

    constexpr Iterator(const Iterator &) noexcept = default; ///< Copy constructor.
    constexpr Iterator(Iterator &&) noexcept = default;      ///< Move constructor.

    // Assignment operators
    constexpr auto operator=(const Iterator &) noexcept -> Iterator & = default; ///< Copy assignment operator.
    constexpr auto operator=(Iterator &&) noexcept -> Iterator & = default;      ///< Move assignment operator.

public:
    // Dereferencing operators
    /**
    * Returns a reference to the data stored in the current node.
    * @brief Dereference operator.
    * @return Reference to the data.
    */
    constexpr auto operator*() noexcept -> reference { return this->m_cursor->m_data; }

    /**
    * Returns a const reference to the data stored in the current node.
    * @brief Dereference operator (const version).
    * @return Const reference to the data.
    */
    constexpr auto operator*() const noexcept -> reference { return this->m_cursor->m_data; }

public:
    // Member access operators
    /**
    * Returns a pointer to the data stored in the current node.
    * @brief Member access operator.
    * @return Pointer to the data.
    */
    constexpr auto operator->() noexcept { return std::addressof(this->m_cursor->m_data); }

    /**
    * Returns a const pointer to the data stored in the current node.
    * @brief Member access operator (const version).
    * @return Const pointer to the data.
    */
    constexpr auto operator->() const noexcept { return std::addressof(this->m_cursor->m_data); }

public:
    // Increment and decrement operators
    /**
    * Advances the iterator to the next node.
    * @brief Pre-increment operator.
    * @return Reference to the updated iterator.
    */
    constexpr auto operator++() noexcept -> iterator &
    {
        this->m_cursor = this->m_cursor->m_next;
        return *this;
    }

    /**
    * Moves the iterator to the previous node.
    * @brief Pre-decrement operator.
    * @return Reference to the updated iterator.
    */
    constexpr auto operator--() noexcept -> iterator &
    {
        this->m_cursor = this->m_cursor->m_prev;
        return *this;
    }

    /**
    * Advances the iterator to the next node and returns a copy of the previous iterator.
    * @brief Post-increment operator.
    * @return Copy of the previous iterator.
    */
    [[nodiscard]] constexpr auto operator++(int) noexcept -> iterator
    {
        auto copy = *this;
        std::ranges::advance(*this, 1);

        return copy;
    }

    /**
    * Moves the iterator to the previous node and returns a copy of the previous iterator.
    * @brief Post-decrement operator.
    * @return Copy of the previous iterator.
    */
    [[nodiscard]] constexpr auto operator--(int) noexcept -> iterator
    {
        auto copy = *this;
        std::ranges::advance(*this, -1);

        return copy;
    }

public:
    // Equality operator
    /**
    * Compares two iterators for equality based on the node they are pointing to.
    * @brief Equality comparison operator.
    * @param lhs Left-hand side iterator.
    * @param rhs Right-hand side iterator.
    * @return True if the iterators point to the same node, false otherwise.
    */
    [[nodiscard]]
    friend constexpr auto operator==(const Iterator &lhs,
                                     const Iterator &rhs) noexcept -> bool
    {
        return (lhs.m_cursor == rhs.m_cursor);
    }

private:
    Node *m_cursor{nullptr}; ///< Pointer to the current node.
};

#endif /* LIST_HPP */
