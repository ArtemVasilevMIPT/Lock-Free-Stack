#pragma once

#include <atomic>

template <class T>
class MPSCQueue {
public:
    // Push adds one element to stack top.
    // Safe to call from multiple threads.
    void Push(const T& value) {
        Node* new_node = new Node(value);
        new_node->LinkBefore(head_.load(std::memory_order_relaxed));
        while (!head_.compare_exchange_weak(new_node->next, new_node, std::memory_order_release,
                                            std::memory_order_relaxed)) {
            // CAS loop
        }
    }

    // Pop removes top element from the stack.
    // Not safe to call concurrently.
    std::pair<T, bool> Pop() {
        if (head_.load(std::memory_order_relaxed) == nullptr) {
            return {{}, false};
        }
        Node* top =
            head_.exchange(head_.load(std::memory_order_relaxed)->next, std::memory_order_acq_rel);
        T val = top->value;
        delete top;
        return {val, true};
    }

    // DequeuedAll Pop's all elements from the stack and calls callback() for each.
    // Not safe to call concurrently with Pop()
    void DequeueAll(auto&& callback) {
        Node* old_head = head_.exchange(nullptr, std::memory_order_acq_rel);
        while (old_head != nullptr) {
            callback(old_head->value);
            auto* tmp = old_head;
            old_head = old_head->next;
            delete tmp;
        }
    }

    ~MPSCQueue() {
        DequeueAll([](const T&) {});
    }

private:
    struct Node {
        T value;
        Node* next;

        Node(const T& v) : value(v) {
        }
        void LinkBefore(Node* ptr) {
            next = ptr;
        }
    };
    std::atomic<Node*> head_{nullptr};
};
