#include "LRUCache.h"
#include "Logger.h"

LRUCache::LRUCache(size_t capacity) : m_capacity(capacity) {
    if (m_capacity == 0) {
        m_capacity = 1; // Enforce minimum capacity to avoid divide-by-zero or logic bugs
    }
}

std::optional<std::string> LRUCache::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cacheMap.find(key);
    if (it == m_cacheMap.end()) {
        LOG_INFO("Cache Miss: " + key);
        return std::nullopt; // Key not found
    }

    // Key found! 
    // We must move the accessed item to the front of the list (most recently used).
    // std::list::splice does this in O(1) without invalidating any iterators or copying data.
    m_items.splice(m_items.begin(), m_items, it->second);
    
    LOG_INFO("Cache Hit: " + key);
    return it->second->second;
}

void LRUCache::put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cacheMap.find(key);
    if (it != m_cacheMap.end()) {
        // Key already exists. Update the value and move it to the front.
        it->second->second = value;
        m_items.splice(m_items.begin(), m_items, it->second);
        return;
    }

    // Key doesn't exist. Check if we are at capacity.
    if (m_cacheMap.size() >= m_capacity) {
        // Evict the least recently used item (which is at the back of the list)
        auto last = m_items.back();
        m_cacheMap.erase(last.first);
        m_items.pop_back();
        LOG_INFO("Cache Evicted: " + last.first);
    }

    // Insert the new item at the front of the list
    m_items.emplace_front(key, value);
    
    // Store the iterator to this new node in the map
    m_cacheMap[key] = m_items.begin();
}
