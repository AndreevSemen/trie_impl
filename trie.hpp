// Copyright 2019 AndreevSemen

#include <iostream>

#ifndef INCLUDE_TRIE_HPP_
#define INCLUDE_TRIE_HPP_

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <utility>
#include <exception>
#include <limits>

template <class T, class KeyType = wchar_t>
class trie
{
private:
    typedef std::basic_string<KeyType> key_string;

    struct trie_node {
        std::pair<KeyType, T> data_;
        bool is_leaf_ = false;

        trie_node* parent_ = nullptr;
        std::vector<trie_node*> children_;


        trie_node() = default;

        trie_node(const trie_node& oth)
          : data_(oth.data_)
          , is_leaf_(oth.is_leaf_)
          , parent_(nullptr)
        {
            for (const auto& child : oth.children_) {
                children_.push_back(new trie_node{*child});
                children_.back()->parent_ = this;
            }
        }

        ~trie_node() {
            for (auto& child : children_) {
                delete child;
            }
        }

        void remove_child(trie_node* child_to_remove) {
            auto found = std::find(children_.begin(), children_.end(),
                                   child_to_remove);

            children_.erase(found);
            delete child_to_remove;
        }

        auto find_by_key(KeyType key_char) {
            auto left = children_.begin();
            auto right = children_.end();

            while (left < right) {
                auto mid = left + std::distance(left, right)/2;

                if ((*mid)->data_.first == key_char) {
                    return mid;
                }

                if ((*mid)->data_.first > key_char) {
                    right = mid;
                } else {
                    left = mid + 1;
                }
            }

            return children_.end();
        }

        void push_child(trie_node* child) {
            children_.push_back(child);

            for (auto r_iter = children_.rbegin(),
                 next_r_iter = r_iter + 1;
                 next_r_iter != children_.rend();
                 ++r_iter, ++next_r_iter) {
                if ((*r_iter)->data_.first >
                    (*next_r_iter)->data_.first) {
                    break;
                }
                std::iter_swap(r_iter, next_r_iter);
            }
        }
    };

    void create_end_prefix() const {
        auto end = new trie_node;

        end->data_.first = std::numeric_limits<KeyType>::max();
        end->parent_ = top_;
        end->is_leaf_ = true;

        top_->children_.push_back(end);
    }

    trie_node* top_;
    size_t size_;

public:
    struct search_iterator {
    private:
        trie_node* node_;

    public:
        explicit search_iterator(trie_node* ptr)
          : node_(ptr)
        {}

        search_iterator& operator=(
                const trie<T, KeyType>::search_iterator& rhs
        ) {
            node_ = rhs.node_;
            return *this;
        }

        const T& value() const {
            return node_->data_.second;
        }
        T& value() {
            return node_->data_.second;
        }

        key_string key() const {
            key_string key_str;

            trie_node* node = node_;
            while (node->parent_ != nullptr) {
                key_str = node->data_.first + key_str;
                node = node->parent_;
            }

            return key_str;
        }

        void advance(const key_string& sub_key) {
            if (sub_key.empty()) {
                throw std::invalid_argument{
                    "Advance with zero prefix"
                };
            }

            auto sub_iter = sub_key.cbegin();
            trie_node* node = node_;

            while (sub_iter != sub_key.cend()) {
                auto found = node->find_by_key(*sub_iter);
                if (found == node->children_.cend()) {
                    throw std::runtime_error{
                        "No such prefix"
                    };
                }
                node = *found;
                ++sub_iter;
            }

            node_->is_leaf_ = false;
            node_ = node;
            node_->is_leaf_ = true;
        }

        std::pair<std::basic_string<KeyType>, T> operator*() const {
            return std::make_pair(key(), node_->data_.second);
        }
        std::pair<std::basic_string<KeyType>, T> operator*() {
            return const_cast<const search_iterator*>(this)->operator*();
        }

        // Arithmetical operators
        search_iterator operator++() {
            if (node_->data_.first == std::numeric_limits<KeyType>::max()) {
                throw std::out_of_range{
                    "Iterator to end couldn't be incremented"
                };
            }

            trie_node* node = node_;

            while (true) {
                if (node->is_leaf_ && node != node_) {
                    return *this = search_iterator(node);
                }

                if (node->parent_->children_.size() > 1) {
                    auto next_child =
                             ++(node->parent_->find_by_key(node->data_.first));

                    if (next_child == node->parent_->children_.end()) {
                        node = node->parent_;
                    } else {
                        node = *next_child;
                        break;
                    }
                } else {
                    node = node->parent_;
                }
            }

            while (true) {
                if (node->is_leaf_ && node->children_.empty()) {
                    return *this = search_iterator(node);
                }

                node = node->children_.front();
            }
        }
        const search_iterator operator++(int) {
            search_iterator old_state(*this);
            operator++();
            return old_state;
        }

        search_iterator operator--() {
            trie_node* node = node_;

            // Shift down
            if (!node->children_.empty()) {
                while (true) {
                    if (node->is_leaf_ && node != node_) {
                        return *this = search_iterator(node);
                    }

                    node = node->children_.back();
                }
            }

            // Shift left
            while (true) {
                if (node->parent_ == nullptr) {
                    throw std::out_of_range{
                        "Begin iterator couldn't be decremented"
                    };
                }

                if (node->parent_->children_.size() > 1) {
                    auto prev_child =
                            node->parent_->find_by_key(node->data_.first);

                    if (prev_child == node->parent_->children_.begin()) {
                        node = node->parent_;
                    } else {
                        node = *(--prev_child);
                        break;
                    }
                } else {
                    node = node->parent_;
                }
            }

            // Shift down
            while (true) {
                if (node->is_leaf_ && node != node_) {
                    return *this = search_iterator(node);
                }

                node = node->children_.back();
            }
        }
        const search_iterator operator--(int) {
            auto old_state(*this);
            operator--();
            return old_state;
        }

        bool operator==(const trie<T, KeyType>::search_iterator& rhs) const {
            return node_ == rhs.node_;
        }
        bool operator!=(const trie<T, KeyType>::search_iterator& rhs) const {
            return node_ != rhs.node_;
        }

        friend trie;
    };

    trie()
      : size_(0)
    {
        top_ = new trie_node;
        top_->is_leaf_ = false;
        top_->parent_ = nullptr;

        create_end_prefix();
    }

    trie(const trie& oth)
      : size_(oth.size_)
    {
        top_ = new trie_node{*oth.top_};
    }

    ~trie() {
        delete top_;
    }

    trie& operator=(const trie<T>& rhs) {
        if (this != &rhs) {
            auto new_top = new trie_node{*rhs.top_};

            std::swap(top_, new_top);
            delete new_top;

            size_ = rhs.size();
        }

        return *this;
    }

    search_iterator begin() const {
        if (empty()) {
            return end();
        }

        trie_node* node = top_;

        while (!node->children_.empty()) {
            node = node->children_.front();
        }

        return search_iterator(node);
    }

    search_iterator end() const {
        return search_iterator{top_->children_.back()};
    }

    size_t size() const { return size_; }
    bool empty() const { return top_->children_.size() == 1; }

    search_iterator insert(const std::pair<key_string, T>& data) {
        if (data.first.empty()) {
            throw std::out_of_range{
                "Empty key couldn't be added"
            };
        }

        auto str_iter = data.first.cbegin();

        auto found = top_->find_by_key(*str_iter);
        trie_node* node = top_;

        if (found != top_->children_.end()) {
            ++str_iter;
            node = *found;

            while (true) {
                if (str_iter == data.first.cend()) {
                    if (!node->is_leaf_) {
                        node->data_.second = data.second;
                        node->is_leaf_ = true;

                        ++size_;

                        return search_iterator(node);
                    }

                    throw std::out_of_range{
                        "Key already exists"
                    };
                }

                found = node->find_by_key(*str_iter);
                if (found == node->children_.end()) {
                    break;
                }

                ++str_iter;
                node = *found;
            }
        }

        while (str_iter != data.first.cend()) {
            auto new_child = new trie_node{};
            new_child->parent_ = node;
            new_child->is_leaf_ = false;
            new_child->data_.first = *str_iter;

            node->push_child(new_child);

            node = new_child;

            ++str_iter;
        }

        node->is_leaf_ = true;
        node->data_.second = data.second;

        ++size_;

        return search_iterator{node};
    }

    void erase(search_iterator iter) {
        if (!iter.node_->children_.empty()) {
            iter.node_->is_leaf_ = false;

            --size_;

            return;
        }

        trie_node* node = iter.node_;

        while (!node->parent_->is_leaf_ &&
               node->parent_->children_.size() <= 1 &&
               node->parent_ != nullptr) {
            node = node->parent_;
        }

        node->parent_->remove_child(node);

        --size_;
    }

    void swap(trie<T, KeyType>& oth) {
        using std::swap;

        swap(top_, oth.top_);
        swap(size_, oth.size_);
    }

    void clear() {
        for (auto& it : top_->children_) {
            delete it;
        }

        top_->children_.clear();
        create_end_prefix();

        size_ = 0;
    }

    search_iterator find(const key_string& key) const {
        auto key_iter = key.cbegin();
        auto node = top_;

        while (key_iter != key.cend()) {
            auto found = node->find_by_key(*key_iter);

            if (found == node->children_.end()) {
                return end();
            }

            if ((*found)->is_leaf_ && key_iter == key.cend() - 1) {
                return search_iterator(*found);
            }

            node = (*found);
            ++key_iter;
        }

        return end();
    }

    bool get_value(const key_string& prefix, T& container) const {
        auto found_iter = find(prefix);

        if (found_iter == end()) {
            return false;
        }

        container = found_iter.value();

        return true;
    }

    search_iterator find_longest_prefix() const {
        size_t max_length = 0;
        auto long_iter = end();

        for (auto iter = begin(); iter != end(); ++iter) {
            size_t length = iter.key().size();

            if (length > max_length) {
                max_length = length;
                long_iter = iter;
            }
        }

        return long_iter;
    }
};

template < typename T, typename KeyType >
void swap(trie<T, KeyType>& lhs, trie<T, KeyType>& rhs) {
    lhs.swap(rhs);
}

#endif // INCLUDE_TRIE_HPP_
