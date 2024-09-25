#ifndef __BINARYSEARCH_HXX__
#define __BINARYSEARCH_HXX__



#include <iostream>
#include <ranges>
#include <utility>


// Namespace containing helper concepts
namespace details
{
    /**
    * @brief Concept to check if a type is not the same as or derived from another type.
    * @tparam T The type to be checked.
    * @tparam U The type to compare against.
    */
    template <class T, class U>
    concept non_self = std::conjunction<
        std::negation<std::bool_constant<std::same_as<std::decay_t<T>, U>>>,
        std::negation<std::bool_constant<std::derived_from<std::decay_t<T>, U>>>>::value;
}


// Templated binary search tree class
template <typename Type> requires std::totally_ordered<Type>
struct binary_search_tree final
{
private:
    // Nested Node struct representing a node in the tree
    struct Node;

private:
    // Type aliases for allocator and allocator traits
    using allocator_t = std::allocator<Node>;
    using traits      = std::allocator_traits<allocator_t>;

public:
    // Public typedefs for types used in the tree
    using value_type = Type;

    using reference       = Type &;
    using const_reference = const Type &;

    using pointer       = Type *;
    using const_pointer = const Type *;

    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

public:
    /**
    * @brief Default constructor.
    */
    constexpr binary_search_tree() noexcept = default;

    /**
    * @brief Copy constructor.
    * @param outer The binary search tree to copy from.
    */
    constexpr binary_search_tree(const binary_search_tree &outer)
        noexcept(std::is_nothrow_copy_constructible<allocator_t>{})
        : m_alloc{traits::select_on_container_copy_construction(outer.m_alloc)}
    {
        this->m_root = this->__M_clone(outer.m_root);
        this->m_size = outer.m_size;
    }

    /**
    * @brief Move constructor.
    * @param outer The binary search tree to move from.
    */
    constexpr binary_search_tree(binary_search_tree &&outer)
        noexcept(std::is_nothrow_move_constructible<allocator_t>{})
        : m_alloc{std::move(outer.m_alloc)}
    {
        this->m_root = std::exchange(outer.m_root, nullptr);
        this->m_size = std::exchange(outer.m_size, 0UL);
    }

    /**
    * @brief Constructor from iterators.
    * @tparam Iterator The iterator type.
    * @tparam Sentinel The sentinel type.
    * @param first The beginning iterator.
    * @param last The ending sentinel.
    */
    template <typename Iterator, typename Sentinel>
        requires std::conjunction<
            std::bool_constant<std::input_iterator<Iterator>>,
            std::bool_constant<std::sentinel_for<Sentinel, Iterator>>
        >::value
    constexpr binary_search_tree(Iterator first, Sentinel last)
        requires(std::constructible_from<Type, std::iter_value_t<Iterator>>)
        : binary_search_tree{}
    {
        this->__M_range_init(first, last);
    }

    /**
    * @brief Constructor from ranges.
    * @tparam range_t The range type.
    * @param range The range to construct the tree from.
    */
    template <typename range_t> requires std::ranges:ranges<range_t>
    constexpr binary_search_tree(std::from_range_t, range_t &&range)
        requires(std::conjunction<
                 std::bool_constant<details::non_self<range_t, binary_search_tree>>,
                 std::bool_constant<std::constructible_from<Type, std::ranges::range_value_t<range_t>>>>::value)
        : binary_search_tree{std::ranges::begin(range), std::ranges::end(range)}
    {
    }

    /**
    * @brief Constructor from initializer list.
    * @param list The initializer list to construct the tree from.
    */
    constexpr binary_search_tree(std::initializer_list<Type> list)
        : binary_search_tree{std::ranges::begin(list), std::ranges::end(list)}
    {
    }

public:
    /**
    * @brief Copy assignment operator.
    * @param rhs The binary search tree to copy from.
    * @return binary_search_tree& Reference to the modified tree.
    */
    constexpr auto operator=(const binary_search_tree &rhs)
            noexcept(std::disjunction<typename traits::propagate_on_container_copy_assignment,
                                        typename traits::is_always_equal>{})
        -> binary_search_tree &
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
     *
     * @param rhs The binary search tree to move from.
     * @return binary_search_tree& Reference to the modified tree.
     */
    constexpr auto operator=(binary_search_tree &&rhs)
            noexcept(std::disjunction<typename traits::propagate_on_container_move_assignment,
                                        typename traits::is_always_equal>{})
        -> binary_search_tree &
    {
        if (this == std::addressof(rhs))
        {
            return *this;
        }

        constexpr bool is_propagate = std::disjunction<
            typename traits::propagate_on_container_move_assignment,
            typename traits::is_always_equal>{};

        this->__M_move_assign(rhs, std::bool_constant<is_propagate>{});

        return *this;
    }

public:
    // Public member functions
    // Insertion and emplacement functions

    // Insert a value into the tree
    template <std::totally_ordered_with<Type> UType>
    constexpr void insert(UType &&data)
        requires std::constructible_from<Type, UType>
    {
        this->__M_insert(this->__M_create_node(std::forward<UType>(data)));
    }

    // Emplace a value into the tree
    template <class... ARGS>
    constexpr void emplace(ARGS &&...args)
        requires std::constructible_from<Type, ARGS...>
    {
        this->__M_insert(
            this->__M_create_node(std::in_place, std::forward_as_tuple(args...)));
    }

public:
    // Search function

    // Search for a value in the tree
    [[nodiscard]] constexpr auto search(Type const &key) noexcept
        -> std::optional<std::reference_wrapper<Type>>
    {
        return __M_search<Type>(this->m_root, key);
    }

    // Const version of search function
    [[nodiscard]] constexpr auto search(Type const &key) const noexcept
        -> std::optional<const std::reference_wrapper<Type>>
    {
        return __M_search<Type>(this->m_root, key);
    }

public:
    // Removal and clearing functions

    // Remove a value from the tree
    constexpr void remove(Type const &key)
    {
        if (auto current = this->m_root)
        {
            while (current != nullptr)
            {
                if (key < current->m_data)
                {
                    current = current->m_left;
                }
                else if (key > current->m_data)
                {
                    current = current->m_right;
                }
                else
                {
                    break;
                }
            }

            if (current == nullptr)
            {
                return;
            }

            auto parent = current->m_parent;

            if ((current->m_left == nullptr) && (current->m_right != nullptr))
            {
                if (parent->m_left == current)
                {
                    parent->m_left = nullptr;
                }
                else
                {
                    parent->m_right = nullptr;
                }

                traits::destroy(this->m_alloc, std::to_address(current));
                traits::deallocate(this->m_alloc, std::to_address(current), 1UL);

                this->m_size = this->m_size - 1UL;

                return;
            }

            if ((current->m_left != nullptr) || (current->m_right != nullptr))
            {
                if (current->m_left != nullptr)
                {
                    parent->m_left = current->m_left;
                    current->m_left->m_parent = parent;
                }
                else
                {
                    parent->m_right = current->m_right;
                    parent->m_right->m_parent = parent;
                }

                traits::destroy(this->m_alloc, std::to_address(current));
                traits::deallocate(this->m_alloc, std::to_address(current), 1UL);

                this->m_size = this->m_size - 1UL;

                return;
            }

            if ((current->m_left != nullptr) && (current->m_right != nullptr))
            {
                auto temp = current->m_right;
                parent = nullptr;

                while (temp->m_left != nullptr)
                {
                    parent = temp;
                    temp = temp->m_left;
                }

                if (parent != nullptr)
                {
                    parent->m_left = temp->m_right;
                }
                else
                {
                    current->m_right = temp->m_right;

                    if (temp->m_right != nullptr)
                    {
                        temp->m_right->m_parent = current;
                    }
                    else
                    {
                        temp->m_parent = current;
                    }
                }

                current->m_data = temp->m_data;

                traits::destroy(this->m_alloc, std::to_address(temp));
                traits::deallocate(this->m_alloc, std::to_address(temp), 1UL);

                this->m_size = this->m_size - 1UL;

                return;
            }
        }
    }

    // Clear the tree
    constexpr void clear()
    {
        Node *current = this->m_root;
        Node *temp = nullptr;

        while (current != nullptr)
        {
            if (current->m_left == nullptr)
            {
                temp = current->m_right;

                traits::destroy(this->m_alloc, std::to_address(current));
                traits::deallocate(this->m_alloc, std::to_address(current), 1UL);
            }
            else
            {
                temp = current->m_left;
                current->m_left = temp->m_right;
                temp->m_right = current;
            }

            current = temp;
        }

        this->m_root = nullptr;
        this->m_size = 0UL;
    }

public:
    // Utility functions

    // Check if the tree is empty
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return (this->m_root == nullptr);
    }

    // Get the size of the tree
    [[nodiscard]] constexpr auto size() const noexcept -> size_type
    {
        return this->m_size;
    }

    // Get the maximum value in the tree
    [[nodiscard]] constexpr auto max() const noexcept -> std::optional<Type>
    {
        if (auto current = this->m_root)
        {
            while (current->m_right != nullptr)
            {
                current = current->m_right;
            }

            return std::optional{current->m_data};
        }

        return std::nullopt;
    }

    // Get the minimum value in the tree
    [[nodiscard]] constexpr auto min() const noexcept -> std::optional<Type>
    {
        if (auto current = this->m_root)
        {
            while (current->m_left != nullptr)
            {
                current = current->m_left;
            }

            return std::optional{current->m_data};
        }

        return std::nullopt;
    }

public:
    // Printing functions

    // Print tree nodes in inorder traversal
    friend constexpr void print_inorder(const binary_search_tree &tree) noexcept
    {
        if (tree.m_root == nullptr)
            return;

        Node *current = tree.m_root;
        Node *temp = nullptr;

        // iterating tree nodes
        while (current != nullptr)
        {
            if (current->m_left == nullptr)
            {
                // Print node value
                std::cout << current->m_data << ' ';
                // When left child are empty then
                // visit to right child
                current = current->m_right;
            }
            else
            {
                temp = current->m_left;
                // Find rightmost node which is
                // equal to current node
                while ((temp->m_right != nullptr) && (temp->m_right != current))
                {
                    // Visit to right subtree
                    temp = temp->m_right;
                }

                if (temp->m_right != nullptr)
                {
                    // Print node value
                    std::cout << current->m_data << ' ';
                    // Unlink
                    temp->m_right = nullptr;
                    // Visit to right child
                    current = current->m_right;
                }
                else
                {
                    // Change link
                    temp->m_right = current;
                    // Visit to right child
                    current = current->m_left;
                }
            }
        }
    }

    // Print tree nodes in preorder traversal
    friend constexpr void print_preorder(const binary_search_tree &tree) noexcept
    {
        if (tree.m_root == nullptr)
            return;

        Node *current = tree.m_root;
        Node *temp = nullptr;

        // iterating tree nodes
        while (current != nullptr)
        {
            if (current->m_left == nullptr)
            {
                // Print node value
                std::cout << current->m_data << ' ';
                // Visit to right childs
                current = current->m_right;
            }
            else
            {
                temp = current->m_left;
                // Find rightmost node which is not
                // equal to current node
                while ((temp->m_right != nullptr) && (temp->m_right != current))
                {
                    // Visit to right subtree
                    temp = temp->m_right;
                }

                if (temp->m_right != current)
                {
                    // Print node value
                    std::cout << current->m_data << ' ';
                    // Connect rightmost right node to current node
                    temp->m_right = current;
                    // Visit to left childs
                    current = current->m_left;
                }
                else
                {
                    // unlink
                    temp->m_right = nullptr;
                    // Visit to right child
                    current = current->m_right;
                }
            }
        }
    }

    // Print tree nodes in postorder traversal
    friend constexpr void print_postorder(const binary_search_tree &tree) noexcept
    {
        if (tree.m_root == nullptr)
            return;

        // Create a dummy node
        Node *dummy = new Node();

        dummy->m_left = tree.m_root;

        auto current = dummy;

        // Define some useful variables
        Node *parent = nullptr;
        Node *middle = nullptr;
        Node *temp = nullptr;
        Node *back = nullptr;

        // iterating tree nodes
        while (current != nullptr)
        {

            if (current->m_left == nullptr)
            {
                // When left child are empty then
                // Visit to right child
                current = current->m_right;
            }
            else
            {
                // Get to left child
                temp = current->m_left;

                while ((temp->m_right != nullptr) && (temp->m_right != current))
                {
                    temp = temp->m_right;
                }

                if (temp->m_right != current)
                {
                    temp->m_right = current;
                    current = current->m_left;
                }
                else
                {
                    parent = current;
                    middle = current->m_left;

                    // Update new path
                    while (middle != current)
                    {
                        back = middle->m_right;
                        middle->m_right = parent;
                        parent = middle;
                        middle = back;
                    }

                    parent = current;
                    middle = temp;

                    // Print the resultant nodes.
                    // And correct node link in current path
                    while (middle != current)
                    {
                        std::cout << middle->m_data << ' ';

                        back = middle->m_right;
                        middle->m_right = parent;
                        parent = middle;
                        middle = back;
                    }

                    // Unlink previous bind element
                    temp->m_right = nullptr;
                    // Visit to right child
                    current = current->m_right;
                }
            }
        }

        delete dummy;
        dummy = nullptr;
    }

    // Swap
    // Swap function

    /**
    * @brief Swaps the contents of two binary search trees.
    * @param lhs The first binary search tree.
    * @param rhs The second binary search tree.
    */
    friend constexpr void swap(binary_search_tree &lhs, binary_search_tree &rhs)
            noexcept(std::disjunction<typename traits::propagate_on_container_swap,
                                        typename traits::is_always_equal>{})
    {
        if constexpr (typename traits::propagate_on_container_swap{})
        {
            std::swap(lhs.m_alloc, rhs.m_alloc);
        }
        std::swap(lhs.m_root, rhs.m_root);
        std::swap(lhs.m_size, rhs.m_size);
    }

public:
    // Destructor
    /**
    * @brief Destructor. Clears the binary search tree.
    */
    constexpr ~binary_search_tree() { this->clear(); }

private:
    // Private member functions

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
    * @brief Search for a value in the tree.
    * @tparam RType The return type of the search.
    * @param current The current node being examined.
    * @param key The key to search for.
    * @return std::optional<RType> An optional containing the found value, if any.
    */
    template <class RType>
    constexpr auto __M_search(const Node *current, const Type &key)
        -> std::optional<RType>
    {
        if (current != nullptr)
        {
            while (current != nullptr && (key != current->m_data))
            {
                if (key == current->m_data)
                    return std::optional{std::ref(current->m_data)};
                else if (key < current->m_data)
                    current = current->m_left;
                else
                    current = current->m_right;
            }
            return std::nullopt;
        }
        return std::nullopt;
    }

    /**
    * @brief Insert a node into the tree.
    * @param node The node to insert.
    */
    constexpr void __M_insert(Node *node)
    {
        if (node == nullptr)
            return;

        if (this->m_root == nullptr)
        {
            this->m_root = node;
            this->m_size = this->m_size + 1UL;
            return;
        }

        Node *current = this->m_root;
        Node *parent = nullptr;

        while (current != nullptr)
        {
            parent = current;
            if (node->m_data <= current->m_data)
                current = current->m_left;
            else
                current = current->m_right;
        }

        node->m_parent = parent;

        if (parent == nullptr)
        {
            parent = node;
            this->m_size = this->m_size + 1UL;
            return;
        }

        if (node->m_data <= parent->m_data)
            parent->m_left = node;
        else
            parent->m_right = node;

        this->m_size = this->m_size + 1UL;
    }

    /**
    * @brief Clone a subtree.
    * @param root The root of the subtree to clone.
    * @return Node* Pointer to the root of the cloned subtree.
    */
    [[nodiscard]] constexpr Node *__M_clone(Node *root)
    {
        if (root != nullptr)
        {
            auto new_root = this->__M_create_node(root->m_data);
            auto cursor = new_root;

            while (root != nullptr)
            {
                if ((root->m_left != nullptr) && (cursor->m_left == nullptr))
                {
                    auto node = this->__M_create_node(root->m_left->m_data);

                    cursor->m_left = node;
                    cursor->m_left->m_parent = cursor;

                    root = root->m_left;
                    cursor = cursor->m_left;
                }
                else if ((root->m_right != nullptr) && (cursor->m_right == nullptr))
                {
                    auto node = this->__M_create_node(root->m_right->m_data);

                    cursor->m_right = node;
                    cursor->m_right->m_parent = cursor;

                    root = root->m_right;
                    cursor = cursor->m_right;
                }
                else
                {
                    root = root->m_parent;
                    cursor = cursor->m_parent;
                }
            }
            return new_root;
        }
        return root;
    }

    /**
    * @brief Initialize tree from a range.
    * @tparam _iterator Iterator type for the range.
    * @tparam _sentinel Sentinel type for the range.
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
        for (; first != last; first = std::ranges::next(first))
        {
            this->insert(*first);
        }
    }

    /**
    * @brief Copy assignment.
    * @param outer The tree to copy from.
    * @param Tag indicating if the allocator is propagate on copy assignment.
    */
    constexpr void __M_copy_assign(const binary_search_tree &outer, std::true_type)
    {
        if constexpr (not typename traits::is_always_equal{})
        {
            if (this->m_alloc != outer.m_alloc)
            {
                this->clear();
            }
            this->m_alloc = outer.m_alloc;
        }

        if (this->m_root != nullptr)
            this->clear();

        this->m_root = this->__M_clone(outer.m_root);
        this->m_size = outer.m_size;
    }

    /**
    * @brief Copy assignment.
    * @param outer The tree to copy from.
    * @param Tag indicating if the allocator is propagate on copy assignment.
    */
    constexpr void __M_copy_assign(const binary_search_tree &outer, std::false_type)
    {
        this->clear();

        this->m_root = this->__M_clone(outer.m_root);
        this->m_size = outer.m_size;
    }

    /**
    * @brief Move assignment.
    * @param outer The tree to move from.
    * @param Tag indicating if the allocator is propagate on move assignment.
    */
    constexpr void __M_move_assign(binary_search_tree &outer, std::true_type)
    {
        this->clear();

        this->m_alloc = std::move(outer.m_alloc);
        this->m_root = std::exchange(outer.m_root, nullptr);
        this->m_size = std::exchange(outer.m_size, 0UL);
    }

    /**
    * @brief Move assignment.
    * @param outer The tree to move from.
    * @param Tag indicating if the allocator is propagate on move assignment.
    */
    constexpr void __M_move_assign(binary_search_tree &outer, std::false_type)
    {
        if (this->m_alloc == outer.m_alloc)
        {
            this->__M_move_assign(outer, std::true_type{});
        }
        else
        {
            this->m_root = std::exchange(outer.m_root, nullptr);
            this->m_size = std::exchange(outer.m_size, 0UL);
        }
    }

private:
    // Private data members
    Node *m_root{nullptr};
    size_type m_size{0UL};
    [[no_unique_address]] allocator_t m_alloc{};
};


// Deduction guides

/**
* @brief Deduction guide for BS_tree from range.
* @tparam range_t Type of the range.
*/
template <class Range_t>
binary_search_tree(std::from_range_t, Range_t &&) -> binary_search_tree<std::ranges::range_value_t<Range_t>>;

/**
* @brief Deduction guide for BS_tree from iterators and sentinels.
* @tparam Iterator Type of the iterator.
* @tparam Sentinel Type of the sentinel.
*/
template <class Iterator, class Sentinel>
binary_search_tree(Iterator, Sentinel) -> binary_search_tree<std::iter_value_t<Iterator>>;



// Node struct representing a node in the binary search tree
template <typename Type> requires std::totally_ordered<Type>
struct binary_search_tree<Type>::Node final
{

public:
    friend struct binary_search_tree;

public:
    // Constructors

    /**
    * @brief Default constructor.
    */
    constexpr Node() noexcept(std::is_nothrow_default_constructible_v<Type>)
        requires(std::default_initializable<Type>)
    = default;

    /**
    * @brief Constructor with data, left, and right nodes.
    * @param data The data to store in the node.
    * @param left Pointer to the left child node.
    * @param right Pointer to the right child node.
    */
    explicit constexpr Node(const Type &data, Node *left = nullptr,
                            Node *right = nullptr) noexcept(std::is_nothrow_copy_constructible_v<Type>)
        requires(std::copy_constructible<Type>)
        : m_data{data}, m_left{left}, m_right{right}
    {
    }

    /**
    * @brief Constructor with data, left, and right nodes (move semantics).
    * @param data The data to store in the node.
    * @param left Pointer to the left child node.
    * @param right Pointer to the right child node.
    */
    explicit constexpr Node(Type &&data, Node *left = nullptr,
                            Node *right = nullptr) noexcept(std::is_nothrow_move_constructible_v<Type>)
        requires(std::move_constructible<Type>)
        : m_data{std::move(data)}, m_left{left}, m_right{right}
    {
    }

    /**
    * @brief Constructor with in-place construction from a tuple.
    * @tparam Tuple The tuple type.
    * @tparam ARGS The argument types of the tuple.
    * @param tuple The tuple to construct the node data from.
    * @param left Pointer to the left child node.
    * @param right Pointer to the right child node.
    */
    template <template <class...> class Tuple, class... ARGS>
    constexpr Node(std::in_place_t, const Tuple<ARGS...> &tuple,
                   Node *left = nullptr,
                   Node *right = nullptr) noexcept(std::is_nothrow_constructible_v<Type, ARGS...>)
        requires(std::constructible_from<Type, ARGS...>)
        : m_data{std::make_from_tuple<Type>(tuple)}, m_left{left}, m_right{right}
    {
    }

private:
    // Private data members
    Type m_data{};
    Node *m_left{nullptr};
    Node *m_right{nullptr};
    Node *m_parent{nullptr};
};

#endif