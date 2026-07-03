#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <string>
#include <unordered_map>
#include <list>
#include <mutex>
#include <optional>

class LRUCache {
public:
    // Initialize cache with a maximum capacity
    explicit LRUCache(size_t capacity);

    // Prevent copying
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    // Retrieve an item from the cache. Returns std::nullopt if not found.
    std::optional<std::string> get(const std::string& key);

    // Insert or update an item in the cache.
    void put(const std::string& key, const std::string& value);

private:
    size_t m_capacity;
    std::mutex m_mutex;

    // The list stores key-value pairs and maintains the "most recently used" order.
    // The front of the list is the most recently used, back is the least recently used.
    std::list<std::pair<std::string, std::string>> m_items;

    // The map stores iterators pointing directly to the list nodes for O(1) access.
    std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>::iterator> m_cacheMap;
};

#endif // LRUCACHE_H
