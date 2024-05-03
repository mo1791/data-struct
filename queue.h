#ifndef __QUEUE_HXX__
#define __QUEUE_HXX__


#include <cassert>
#include <ranges>
#include <utility>


/**
* @brief A generic queue implemented as a doubly linked list.
* @tparam Type The type of elements stored in the queue.
*/
template <class Type>
struct queue final
{

private:
    /**
    * @brief Nested struct representing a node in the queue.
    */
    struct Node;

public:
    // Public member type aliases
    using value_type = Type;

    using reference       = Type &;
    using const_reference = const Type &;

    using pointer       = Type *;
    using const_pointer = const Type *;

    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

private:
    // Allocator and allocator traits
    using allocator_t = std::allocator<Node>;
    using traits      = std::allocator_traits<allocator_t>;

public:
    // Constructors

    /**
    * @brief Default constructor.
    * Constructs an empty queue.
    */
    constexpr queue()
        requires std::default_initializable<Type>
    {
        this->m_head = this->__M_create_node();
    }

    /**
    * @brief Copy constructor.
    * Constructs the queue with the contents of `other`.
    * @param outer The queue to be copied.
    */
    constexpr queue(const queue &outer) noexcept(std::is_nothrow_copy_constructible<allocator_t>{})
        : m_alloc{traits::select_on_container_copy_construction(outer.m_alloc)}
    {
        this->m_head = this->__M_clone(outer.m_head->m_next);
        this->m_size = outer.m_size;
    }

    /**
    * @brief Move constructor.
    * Constructs the queue by moving the contents of `other`.
    * @param outer The queue to be moved.
    */
    constexpr queue(queue &&outer) noexcept
        : m_alloc{std::move(outer.m_alloc)}
    {
        this->m_head = std::exchange(outer.m_head, this->__M_create_node());
        this->m_size = std::exchange(outer.m_size, 0UL);
    }

    /**
    * @brief Initializer list constructor.
    * Constructs the queue with elements from an initializer list.
    * @param list The initializer list containing elements.
    */
    constexpr queue(std::initializer_list<Type> list)
        : queue{std::ranges::begin(list), std::ranges::end(list)}
    {
    }

    /**
    * @brief Range constructor.
    * Constructs the queue from a range.
    * @tparam range_t Type of the range.
    * @param from_range_t Tag type for disambiguation.
    * @param range The range to construct the queue from.
    */
    template <std::ranges::range range_t>
    constexpr queue(std::from_range_t, range_t &&range)
        requires std::constructible_from<Type, std::ranges::range_value_t<range_t>>
        : queue{std::ranges::begin(range), std::ranges::end(range)}
    {
    }

    /**
    * @brief Iterator range constructor.
    * Constructs the queue from iterators and sentinels.
    * @tparam Iterator Type of the iterator.
    * @tparam Sentinel Type of the sentinel.
    * @param first The beginning of the range.
    * @param last The end of the range.
    */
    template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
    constexpr queue(Iterator first, Sentinel last)
        requires std::constructible_from<Type, std::iter_value_t<Iterator>>
        : queue{}
    {
        this->__M_range_init(first, last);
    }

    // Assignment operators

    /**
    * @brief Copy assignment operator.
    * Assigns the contents of `rhs` to the queue.
    * @param rhs The queue to be copied.
    * @return Reference to the assigned queue.
    */
    constexpr queue &operator=(const queue &rhs)
                noexcept(std::disjunction<typename traits::propagate_on_container_copy_assignment,
                                            typename traits::is_always_equal>{})
    {
        if (this == std::addressof(rhs)) {
            return *this;
        }

        this->__M_copy_assign(rhs, typename traits::propagate_on_container_copy_assignment{});
        return *this;
    }

    /**
    * @brief Move assignment operator.
    * Moves the contents of `rhs` to the queue.
    * @param rhs The queue to be moved.
    * @return Reference to the moved queue.
    */
    constexpr queue &operator=(queue &&rhs)
                noexcept(std::disjunction<typename traits::propagate_on_container_copy_assignment,
                                            typename traits::is_always_equal>{})
    {
        if (this == std::addressof(rhs)) {
            return *this;
        }

        constexpr bool _is_propagate = std::disjunction<typename traits::propagate_on_container_move_assignment,
                                                        typename traits::is_always_equal>{};

        this->__M_move_assign(rhs, std::bool_constant<_is_propagate>{});
        return *this;
    }

public:
    // Modifiers

    /**
    * @brief Adds an element to the back of the queue.
    * @tparam UType Type of the element to be added.
    * @param data The element to be added.
    */
    template <class UType>
    constexpr void push_back(UType &&data)
        requires std::constructible_from<Type, UType>
    {
        assert(this->m_head != nullptr);

        this->m_head->push_back(this->__M_create_node(std::forward<UType>(data)));
        this->m_size = this->m_size + 1UL;
    }

    /**
    * @brief Constructs an element in-place at the back of the queue.
    * @tparam ARGS Types of the arguments to construct the element.
    * @param args Arguments to construct the element.
    */
    template <class... ARGS>
    constexpr void emplace_back(ARGS &&...args)
        requires std::constructible_from<Type, ARGS...>
    {
        assert(this->m_head != nullptr);

        this->m_head->push_back(this->__M_create_node(std::in_place, std::forward<ARGS>(args)...));
        this->m_size = this->m_size + 1UL;
    }

public:
    // Element Access

    /**
    * @brief Returns an optional reference to the first element.
    * @return Optional reference to the first element.
    */
    constexpr auto front() noexcept -> std::optional<std::reference_wrapper<Type>>
    {
        if (this->empty()) {
            return std::nullopt;
        }
        return std::optional{std::ref(this->m_head->m_next->m_data)};
    }

    /**
    * @brief Returns a const optional reference to the first element.
    * @return Const optional reference to the first element.
    */
    constexpr auto front() const noexcept -> std::optional<const std::reference_wrapper<Type>>
    {
        if (this->empty()) {
            return std::nullopt;
        }
        return std::optional{std::cref(this->m_head->m_next->m_data)};
    }

    /**
    * @brief Returns an optional reference to the last element.
    * @return Optional reference to the last element.
    */
    constexpr auto back() noexcept -> std::optional<std::reference_wrapper<Type>>
    {
        if (this->empty()) {
            return std::nullopt;
        }
        return std::optional{std::ref(this->m_head->m_prev->m_data)};
    }

    /**
    * @brief Returns a const optional reference to the last element.
    * @return Const optional reference to the last element.
    */
    constexpr auto back() const noexcept -> std::optional<const std::reference_wrapper<Type>>
    {
        if (this->empty()) {
            return std::nullopt;
        }
        return std::optional{std::cref(this->m_head->m_prev->m_data)};
    }

public:
    // Capacity

    /**
     * @brief Checks if the queue is empty.
     * @return True if the queue is empty, otherwise false.
     */
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return ((this->m_head != nullptr) || ((this->m_head == this->m_head->m_prev) && 
                                              (this->m_head == this->m_head->m_next)));
    }

    /**
    * @brief Returns the size of the queue.
    * @return Size of the queue.
    */
    [[nodiscard]] constexpr auto size() const noexcept -> size_type { return this->m_size; }

public:
    // Friend Function

    /**
    * @brief Swaps the contents of two queues.
    * @param lhs The first queue.
    * @param rhs The second queue.
    */
    friend constexpr void swap(queue &lhs, queue &rhs)
                noexcept(std::disjunction<typename traits::propagate_on_container_swap,
                                            typename traits::is_always_equal>{})
    {
        if constexpr (typename traits::propagate_on_container_swap{}) {
            std::swap(lhs.m_alloc, rhs.m_alloc);
        }

        std::swap(lhs.m_head, rhs.m_head);
        std::swap(lhs.m_size, rhs.m_size);
    }

public:
    // Destructor

    /**
    * @brief Destroys the queue, deallocating memory.
    */
    constexpr ~queue()
    {
        this->clear();
        traits::destroy(this->m_alloc, this->m_head);
        traits::deallocate(this->m_alloc, this->m_head, 1UL);
        this->m_head = nullptr;
    }

private:
    // Helper functions

    /**
    * @brief Creates a node with provided arguments.
    * @tparam ARGS Types of the arguments.
    * @param args Arguments to construct the node.
    * @return Pointer to the created node.
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
    * @brief Clones the nodes from the provided source.
    * @param source Pointer to the source node.
    * @return Pointer to the cloned nodes.
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
    * @brief Initializes the queue with elements from a range.
    * @tparam _iterator Type of the iterator.
    * @tparam _sentinel Type of the sentinel.
    * @param first The beginning of the range.
    * @param last The end of the range.
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
    * @brief Assigns the nodes from the provided source to the queue.
    * @param source Pointer to the source node.
    * @param size Size of the source.
    */
    constexpr void __M_assign(Node *source, size_type size)
    {
        if (source == nullptr)
        {
            this->clear();
            return;
        }

        if (this->empty())
        {
            if (this->m_head != nullptr)
            {
                traits::destroy(this->m_alloc, this->m_head);
                traits::deallocate(this->m_alloc, this->m_head, 1UL);
                this->m_head = nullptr;
            }

            this->m_head = this->__M_clone(source->m_next);
            return;
        }

        auto cursor = this->m_head->m_next;

        for (auto count = std::min(this->m_size, size); count; --count)
        {
            cursor->m_data = source->m_data;

            cursor = cursor->m_next;
            source = source->m_next;
        }

        if (this->m_size == size)
            return;

        if (this->m_size < size)
        {
            for (auto count = size - this->m_size; count; --count)
            {
                cursor->push_back(this->__M_create_node(source->m_data));

                source = source->m_next;
            }

            return;
        }

        if (this->m_size > size)
        {
            for (auto count = this->m_size - size; count; --count)
            {
                auto target = cursor;

                cursor->m_prev->m_next = cursor->m_next;
                cursor->m_next->m_prev = cursor->m_prev;

                cursor = cursor->m_next;

                traits::destroy(this->m_alloc, target);
                traits::deallocate(this->m_alloc, target, 1UL);
            }
        }
    }

    /**
    * @brief Helper function for copy assignment.
    * @param outer The queue to be copied.
    * @param Tag Whether to propagate on copy assignment.
    */
    constexpr void __M_copy_assign(const queue &outer, std::true_type)
    {
        if constexpr (not typename traits::is_always_equal{}) {

            if (this->m_alloc != outer.m_alloc) {
                this->clear();
            }
            this->m_alloc = outer.m_alloc;
        }

        this->__M_assign(outer.m_head, outer.m_size);
        this->m_size = outer.m_size;
    }

    /**
     * @brief Helper function for copy assignment.
     *
     * @param outer The queue to be copied.
     * @param Tag Whether to propagate on copy assignment.
     */
    constexpr void __M_copy_assign(const queue &outer, std::false_type)
    {
        this->clear();
        this->__M_assign(outer.m_head, outer.m_size);
        this->m_size = outer.m_size;
    }

    /**
     * @brief Helper function for move assignment.
     *
     * @param outer The queue to be moved.
     * @param Tag Whether to propagate on move assignment.
     */
    constexpr void __M_move_assign(queue &outer, std::true_type)
    {
        this->clear();

        this->m_alloc = std::move(outer.m_alloc);
        this->m_head = std::exchange(outer.m_head, this->__M_create_node());
        this->m_size = std::exchange(outer.m_size, 0UL);
    }

    /**
     * @brief Helper function for move assignment.
     *
     * @param outer The queue to be moved.
     * @param Tag Whether to propagate on move assignment.
     */
    constexpr void __M_move_assign(queue &outer, std::false_type)
    {
        if (this->m_alloc == outer.m_alloc) {
            this->__M_move_assign(outer, std::true_type{});
        }
        else {
            this->m_head = std::exchange(outer.m_head, this->__M_create_node());
            this->m_size = std::exchange(outer.m_size, 0UL);
        }
    }

private:
    // Member variables
    Node*     m_head{nullptr};
    size_type m_size{0UL};
    [[no_unique_address]] allocator_t m_alloc{};
};

// Deduction guides

/**
* @brief Deduction guide for queue from range.
* @tparam range_t Type of the range.
*/
template <class range_t>
queue(std::from_range_t, range_t &&) -> queue<std::ranges::range_value_t<range_t>>;

/**
* @brief Deduction guide for queue from iterators and sentinels.
* @tparam Iterator Type of the iterator.
* @tparam Sentinel Type of the sentinel.
*/
template <class Iterator, class Sentinel>
queue(Iterator, Sentinel) -> queue<std::iter_value_t<Iterator>>;



/**
* This class template represents a node in a queue, containing a piece of data
* of type Type and pointers to the previous and next nodes in the queue.
* @brief Represents an individual node in a queue.
* @tparam Type The type of data held by the node.
*/
template <class Type>
struct queue<Type>::Node final
{

public:
    friend struct queue;

public:
    // Constructors

    /**
    * @brief Default constructor.
    * Constructs an empty node.
    */
    constexpr Node() noexcept(std::is_nothrow_default_constructible_v<Type>)
        requires std::default_initializable<Type>
    = default;

    /**
    * @brief Constructs a node with provided data.
    * @param data The data to be stored in the node.
    */
    explicit constexpr Node(Type const &data) noexcept(std::is_nothrow_copy_constructible_v<Type>)
        requires std::copy_constructible<Type>
        : m_data{data}
    {
    }

    /**
    * @brief Move constructor.
    * Constructs a node by moving the provided data.
    * @param data The data to be stored in the node.
    */
    explicit constexpr Node(Type &&data) noexcept(std::is_nothrow_move_constructible_v<Type>)
        requires std::move_constructible<Type>
        : m_data{std::move(data)}
    {
    }

    /**
    * @brief In-place constructor.
    * Constructs a node in-place with provided arguments.
    * @tparam ARGS Types of the arguments.
    * @param args Arguments to construct the node.
    */
    template <class... ARGS>
    constexpr Node(std::in_place_t, ARGS &&...args) noexcept(std::is_nothrow_constructible_v<Type, ARGS...>)
        requires std::constructible_from<Type, ARGS...>
        : m_data{std::forward<ARGS>(args)...}
    {
    }

private:
    // Helper function to add a node after the current node.
    constexpr void push_back(Node *node) noexcept
    {
        node->m_prev = this->m_prev;
        node->m_next = this;
        this->m_prev->m_next = node;
        this->m_prev = node;
    }

private:
    // Member variables
    Type m_data{};      // Data stored in the node
    Node *m_prev{this}; // Pointer to the previous node
    Node *m_next{this}; // Pointer to the next node
};

