//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#ifndef GPU2_INTERNAL_H_
#define GPU2_INTERNAL_H_

#include "gpu2.h"

#include <cstdlib>
#include <iostream>
#include <stack>
#include <vector>

#ifndef NDEBUG
    #define GPU2_ASSERT(cond, msg) \
        do { \
            if (!(cond)) { \
                std::cerr << "Assertion failed: " #cond ", " \
                          << "message: " << msg << ", " \
                          << "file: " << __FILE__ << ", " \
                          << "line: " << __LINE__ << std::endl; \
                std::abort(); \
            } \
        } while(false)
#else
    #define GPU2_ASSERT(cond, msg) ((void) 0)
#endif // NDEBUG

namespace gpu {

template<typename T>
class Pool {
public:
    using key_type = HandleType;
    using value_type = T;

private:
    static constexpr key_type KeyMask = 1;

    std::vector<std::pair<value_type, bool>> m_data = {};
    std::stack<key_type> m_free = {};

public:
    Pool() = default;

    /// Returns the value in this pool with the given key, if it exists.
    const value_type& get(key_type k) const {
        GPU2_ASSERT(contains(k), "Invalid handle!");
        return m_data[k - KeyMask].first;
    }

    value_type& get(key_type k) {
        GPU2_ASSERT(contains(k), "Invalid handle!");
        return m_data[k - KeyMask].first;
    }

    /// Add the given value to this pool, and returns a key to it.
    key_type add(value_type v) {
        key_type k;

        if (m_free.empty()) {
            k = static_cast<key_type>(m_data.size());
            m_data.push_back({ std::move(v), true });
        } else {
            k = m_free.top();
            m_free.pop();
            m_data[k] = { std::move(v), true };
        }

        return k + KeyMask;
    }

    /// Removes and returns the entry with the given key, if it exists.
    value_type remove(key_type k) {
        GPU2_ASSERT(contains(k), "Invalid handle!");

        k -= KeyMask;

        value_type v = std::move(m_data[k].first);
        m_data[k].second = false;
        m_free.push(k);

        return v;
    }

    /// Returns true if this pool contains an entry for the given key.
    bool contains(key_type k) const {
        return (k - KeyMask) < m_data.size() && m_data[(k - KeyMask)].second;
    }

    /// Returns true if the contents of this pool is empty.
    bool empty() const {
        return (m_data.size() - m_free.size()) == 0;
    }
};

} // namespace gpu

#endif // GPU2_INTERNAL_H_
