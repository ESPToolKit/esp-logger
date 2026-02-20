#pragma once

#if __has_include(<ESPBufferManager.h>)
#include <ESPBufferManager.h>
#define ESP_LOGGER_HAS_BUFFER_MANAGER 1
#elif __has_include(<esp_buffer_manager/buffer_manager.h>)
#include <esp_buffer_manager/buffer_manager.h>
#define ESP_LOGGER_HAS_BUFFER_MANAGER 1
#else
#define ESP_LOGGER_HAS_BUFFER_MANAGER 0
#endif

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>

namespace logger_allocator_detail {
inline void *allocate(std::size_t bytes, bool usePSRAMBuffers) noexcept {
#if ESP_LOGGER_HAS_BUFFER_MANAGER
    return ESPBufferManager::allocate(bytes, usePSRAMBuffers);
#else
    (void)usePSRAMBuffers;
    return std::malloc(bytes);
#endif
}

inline void deallocate(void *ptr) noexcept {
#if ESP_LOGGER_HAS_BUFFER_MANAGER
    ESPBufferManager::deallocate(ptr);
#else
    std::free(ptr);
#endif
}
}  // namespace logger_allocator_detail

template <typename T>
class LoggerAllocator {
   public:
    using value_type = T;

    LoggerAllocator() noexcept = default;
    explicit LoggerAllocator(bool usePSRAMBuffers) noexcept : _usePSRAMBuffers(usePSRAMBuffers) {}

    template <typename U>
    LoggerAllocator(const LoggerAllocator<U> &other) noexcept : _usePSRAMBuffers(other.usePSRAMBuffers()) {}

    T *allocate(std::size_t n) {
        if (n == 0) {
            return nullptr;
        }
        if (n > (std::numeric_limits<std::size_t>::max() / sizeof(T))) {
#if defined(__cpp_exceptions)
            throw std::bad_alloc();
#else
            std::abort();
#endif
        }

        void *memory = logger_allocator_detail::allocate(n * sizeof(T), _usePSRAMBuffers);
        if (memory == nullptr) {
#if defined(__cpp_exceptions)
            throw std::bad_alloc();
#else
            std::abort();
#endif
        }
        return static_cast<T *>(memory);
    }

    void deallocate(T *ptr, std::size_t) noexcept {
        logger_allocator_detail::deallocate(ptr);
    }

    bool usePSRAMBuffers() const noexcept {
        return _usePSRAMBuffers;
    }

    template <typename U>
    bool operator==(const LoggerAllocator<U> &other) const noexcept {
        return _usePSRAMBuffers == other.usePSRAMBuffers();
    }

    template <typename U>
    bool operator!=(const LoggerAllocator<U> &other) const noexcept {
        return !(*this == other);
    }

   private:
    template <typename>
    friend class LoggerAllocator;

    bool _usePSRAMBuffers = false;
};
