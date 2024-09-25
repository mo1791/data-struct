#ifndef __STACK_HXX__
#define __STACK_HXX__

#include <ranges>
#include <utility>



/**
* @brief A stack data structure implemented using a singly linked list.
* @tparam Type The type of elements stored in the stack.
*/
template <class Type>
struct stack final
{

private:
    // Forward declaration of the Node structure
    struct Node;

private:
    // Type aliases for allocator and allocator traits
    using allocator_type = std::allocator<Node>;
    using traits         = std::allocator_traits<allocator_type>;

public:
    // Type aliases for readability and standard conformance
    using value_type = Type;

    using reference       = Type &;
    using const_reference = const Type &;

    using pointer       = Type *;
    using const_pointer = const Type *;

    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

public:
    // Constructors

    /**
    * @brief Default constructor for stack.
    */
    constexpr stack() noexcept = default;

    /**
    * @brief Move constructor for stack.
    * @param outer The stack to be moved from.
    */
    constexpr stack(stack &&outer) noexcept(std::is_nothrow_move_constructible_v<allocator_type>)
        : m_alloc{std::move(outer.m_alloc)}
    {
        this->m_head = std::exchange(outer.m_head, nullptr);
        this->m_size = std::exchange(outer.m_size, 0UL);
    }

    /**
    * @brief Copy constructor for stack.
    * @param outer The stack to be copied from.
    */
    constexpr stack(const stack &outer)
        : m_alloc{traits::select_on_copy_construction(outer.m_alloc)}
    {
        this->m_head = this->__M_clone(outer.m_head);
        this->m_size = outer.m_size;
    }

    /**
    * @brief Range constructor using iterators.
    * @tparam Iterator The type of iterator.
    * @tparam Sentinel The type of sentinel.
    * @param begin The beginning iterator of the range.
    * @param end The ending sentinel of the range.
    */
    template <typename Iterator, typename Sentinel>
        requires std::conjunction<
            std::bool_constant<std::input_iterator<Iterator>>,
            std::bool_constant<std::sentinel_for<Sentinel, Iterator>>,
            std::bool_constant<std::constructible_from<Type, std::iter_value_t<Iterator>>
        >::value
    constexpr stack(Iterator begin, Sentinel end)
        : stack{}
    {
        while (begin != end)
        {
            this->push(*begin);
            begin = std::ranges::next(std::move(begin));
        }
    }

    /**
    * @brief Initializer list constructor.
    * @param list The initializer list of elements.
    */
    constexpr stack(std::initializer_list<Type> list)
        : stack{std::ranges::begin(list), std::ranges::end(list)}
    {
    }

    /**
    * @brief Range constructor using ranges.
    * @tparam range_t The type of range.
    * @param from_range_t A tag type to identify the range constructor.
    * @param range The range of elements.
    */
    template <typename range_t>
        requires std::conjunction<
            std::bool_constant<std::ranges::range<range_t>>,
            std::bool_constant<std::constructible_from<Type, std::ranges::range_value_t<range_t>>
        >::value
    constexpr stack(std::from_range_t, range_t &&range)
        : stack{std::ranges::begin(range), std::ranges::end(range)}
    {
    }

public:
    /**
    * @brief Copy assignment operator.
    * @param rhs The stack to be copied from.
    * @return Reference to the newly assigned stack.
    */
    constexpr stack &operator=(const stack &rhs)
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
     *
     * @param rhs The stack to be moved from.
     * @return Reference to the newly assigned stack.
     */
    constexpr stack &operator=(stack &&rhs)
                noexcept(std::disjunction<typename traits::propagate_on_container_move_assignment,
                                            typename traits::is_always_equal>{})
    {
        if (this == std::addressof(rhs)) {
            return *this;
        }

        constexpr bool is_propagate = std::disjunction<
            typename traits::propagate_on_container_move_assignment,
            typename traits::is_always_equal>::value;

        this->__M_move_assign(rhs, std::bool_constant<is_propagate>());

        return *this;
    }

public:
    /**
    * @brief Push an element onto the top of the stack.
    * @tparam UType The type of the element to be pushed.
    * @param data The data of the element to be pushed.
    */
    template <class UType>
    constexpr void push(UType &&data)
        requires(std::constructible_from<Type, UType>)
    {
        this->m_head = this->__M_create_node(std::forward<UType>(data), this->m_head);
        this->m_size = this->m_size + 1UL;
    }

    /**
    * @brief Emplace an element onto the top of the stack.
    * @tparam ARGS The types of arguments for constructing the element.
    * @param args The arguments for constructing the element.
    */
    template <class... ARGS>
    constexpr void emplace(ARGS &&...args)
        requires(std::constructible_from<Type, ARGS...>)
    {
        this->m_head = this->__M_create_node(std::in_place, std::forward_as_tuple(args...), this->m_head);
        this->m_size = this->m_size + 1UL;
    }

public:
    /**
    * @brief Pop the top element from the stack.
    */
    constexpr void pop()
    {
        if (auto target = this->m_head)
        {
            this->m_head = this->m_head->m_next;

            traits::destroy(this->m_alloc, std::to_address(target));
            traits::deallocate(this->m_alloc, std::to_address(target), 1UL);

            this->m_size = this->m_size - 1UL;
        }
    }

    /**
    * @brief Clear all elements from the stack.
    */
    constexpr void clear()
    {
        while (this->m_head != nullptr)
            this->pop();
    }

public:
    /**
    * @brief Get the top element of the stack (const version).
    * @return Optional containing the top element if the stack is not empty, otherwise std::nullopt.
    */
    [[nodiscard]]
    constexpr auto top() const noexcept -> std::optional<const std::reference_type<Type>>
    {
        if (this->m_head == nullptr) {
            return std::nullopt;
        }
        return std::optional{std::cref(this->m_head->m_data)};
    }

    /**
    * @brief Get the top element of the stack.
    * @return Optional containing the top element if the stack is not empty, otherwise std::nullopt.
    */
    [[nodiscard]]
    constexpr auto top() noexcept -> std::optional<std::reference_type<Type>>
    {
        if (this->m_head == nullptr) {
            return std::nullopt;
        }
        return std::optional{std::ref(this->m_head->m_data)};
    }

public:
    /**
    * @brief Check if the stack is empty.
    * @return True if the stack is empty, otherwise false.
    */
    [[nodiscard]] constexpr auto empty() const noexcept(true) -> bool
    {
        return (this->m_head == nullptr);
    }

    /**
    * @brief Get the size of the stack.
    * @return The number of elements in the stack.
    */
    [[nodiscard]] constexpr auto size() const noexcept -> size_type { return this->m_size; }

public:
    /**
    * @brief Swap the contents of two stacks.
    * @param lhs The first stack.
    * @param rhs The second stack.
    */
    friend constexpr void swap(stack &lhs, stack &rhs)
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
    /**
    * @brief Destructor for the stack.
    */
    constexpr ~stack() { this->clear(); }

private:
    /**
    * @brief Helper method to create a new node.
    * @tparam ARGS The types of arguments for constructing the node.
    * @param args The arguments for constructing the node.
    * @return Pointer to the newly created node.
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
    * @brief Helper method to clone a node and its subsequent nodes.
    * @param source The source node to be cloned.
    * @return Pointer to the cloned node.
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
    * @brief Assigns elements from another stack to this stack, maintaining the size.
    * @param source Pointer to the head of the source stack.
    * @param size The size of the source stack.
    */
    constexpr void __M_assign(Node *source, std::size_t size)
    {
        if (source == nullptr)
        {
            this->clear();
            return;
        }

        if (this->m_head == nullptr)
        {
            this->m_head = this->__M_clone(source);
            return;
        }

        Node *cursor = this->m_head;
        Node *temp = nullptr;

        for (auto count = std::min(this->m_size, size); count; count--)
        {
            cursor->m_data = source->m_data;

            temp   = cursor;
            cursor = cursor->m_next;
            source = source->m_next;
        }

        if (this->m_size > size)
        {
            temp->m_next = nullptr;

            for (auto count = this->m_size - size; count; --count)
            {
                temp = cursor;
                cursor = cursor->m_next;
                traits::destroy(this->m_alloc, std::to_address(temp));
                traits::deallocate(this->m_alloc, std::to_address(temp), 1UL);
            }
            return;
        }

        if (this->m_size < size)
        {
            for (auto count = size - this->m_size; count; --count)
            {
                temp->m_next = this->__M_create_node(source->m_data);
                temp = temp->m_next;
                source = source->m_next;
            }
        }
    }

    /**
    * @brief Copy assigns elements from another stack to this stack.
    * @param outer The source stack to copy from.
    * @param Tag Whether to propagate on container copy assignment.
    */
    constexpr void __M_copy_assign(const stack &outer, std::true_type)
    {
        if constexpr (not typename traits::is_always_equal{})
        {
            if (this->m_alloc != outer.m_alloc)
            {
                this->clear();
            }
            this->m_alloc = outer.m_alloc;
        }

        this->__M_assign(outer.m_head, outer.m_size);
        this->m_size = outer.m_size;
    }

    /**
    * @brief Copy assigns elements from another stack to this stack.
    * @param outer The source stack to copy from.
    * @param Tag Whether to propagate on container copy assignment.
    */
    constexpr void __M_copy_assign(const stack &outer, std::false_type)
    {
        this->clear();
        this->__M_assign(outer.m_head, outer.m_size);
        this->m_size = outer.m_size;
    }

    /**
    * @brief Move assigns elements from another stack to this stack.
    * @param outer The source stack to move from.
    * @param  Tag wether to propagate on container move assignment.
    */
    constexpr void __M_move_assign(stack &outer, std::true_type)
    {
        this->clear();
        this->m_alloc = std::move(outer.m_alloc);
        this->m_head = std::exchange(outer.m_head, nullptr);
        this->m_size = std::exchange(outer.m_size, 0L);
    }

    /**
    * @brief Move assigns elements from another stack to this stack.
    * @param outer The source stack to move from.
    * @param Tag Whether to propagate on container move assignment.
    */
    constexpr void __M_move_assign(stack &outer, std::false_type)
    {
        if (this->m_alloc == outer.m_alloc)
        {
            this->__M_move_assign(outer, std::true_type{});
        }
        else
        {
            this->m_head = std::exchange(outer.m_head, nullptr);
            this->m_size = std::exchange(outer.m_size, 0L);
        }
    }

private:
    //  Pointer to the head of the stack
    Node *m_head{nullptr};

    //  The size of the stack
    std::size_t m_size{0UL};

    //  The allocator object
    [[no_unique_address]] allocator_type m_alloc{};
};


// Additional user-defined type deduction guides...

/**
* @brief Deduction guide for stack from range.
* @tparam range_t Type of the range.
*/
template <typename range_t>
stack(std::from_range_t, range_t &&) -> stack<std::ranges::range_value_t<range_t>>;

/**
* @brief Deduction guide for stack from iterators and sentinels.
* @tparam Iterator Type of the iterator.
* @tparam Sentinel Type of the sentinel.
*/
template <typename iterator, typename sentinel>
stack(iterator, sentinel) -> stack<std::iter_value_t<iterator>>;

// End of stack structure



/**
* @brief Nested struct representing a node in the stack.
* @tparam Type The type of elements stored in the node.
*/
template <class Type>
struct stack<Type>::Node final
{

public:
    friend struct stack;

public:
    /**
    * @brief Default constructor for Node.
    */
    constexpr Node() noexcept(std::is_nothrow_default_constructible_v<Type>)
        requires std::default_initializable<Type>
    = default;

    /**
    * @brief Constructor for Node with data and next pointer.
    * @param data The data to be stored in the node.
    * @param next Pointer to the next node in the stack.
    */
    explicit constexpr Node(const Type &data, Node *next = nullptr)
        noexcept(std::is_nothrow_copy_constructible_v<Type>)
        requires std::copy_constructible<Type>
        : m_data{data}, m_next{next}
    {
    }

    /**
    * @brief Constructor for Node with rvalue data and next pointer.
    * @param data The rvalue data to be stored in the node.
    * @param next Pointer to the next node in the stack.
    */
    explicit constexpr Node(Type &&data, Node *next = nullptr)
        noexcept(std::is_nothrow_move_constructible_v<Type>)
        requires std::move_constructible<Type>
        : m_data{std::move(data)}, m_next{next}
    {
    }

    /**
    * @brief Constructor for Node using tuple arguments.
    * @tparam Tuple The type of tuple.
    * @tparam ARGS The types of arguments in the tuple.
    * @param tag An in-place tag for construction.
    * @param tuple The tuple containing arguments for constructing the data.
    * @param next Pointer to the next node in the stack.
    */
    template <template <class...> class Tuple, class... ARGS>
    constexpr Node(std::in_place_t, const Tuple<ARGS...> &tuple, Node *next = nullptr)
        noexcept(std::is_nothrow_constructible_v<Type, ARGS...>)
        requires std::constructible_from<Type, ARGS...>
        : m_data{std::make_from_tuple<Type>(tuple)}, m_next{next}
    {
    }

    // Data members for the value and the next pointer
    Type m_data{};
    Node *m_next{nullptr};
};

#endif
