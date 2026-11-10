#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"



/**
 * @brief Class manage a topic by sub/pub policy (uORB)
 * @tparam T data struct of message 
 * @tparam MAX_WATCHERS Number of task can regis and wait on Topic
 */

template <typename T, size_t MAX_WATCHERS = 4>
class OrbTopic
{
    // Dam bao struct t khong chua con tro cap phat dong (nhu std::vector, std::string)
    // De viec sao chet ben trong Spinlock dien ra tuc thoi va an toan bo nho
    static_assert(std::is_trivially_copyable<T>::value,
                    "OrbTopic message struct must be trivially copyable!");

    
    public:
    OrbTopic() = default;

    OrbTopic(const OrbTopic&) = delete;
    OrbTopic& operator=(const OrbTopic&) = delete;

    template <typename, size_t>
    friend class OrbSubscription;

    
    /**
     * @brief (Publish) new data to Topic (callback from FreeRTOS Task)
     */
    void publish(const T& new_data);

    /**
     * @brief Publish directly from hardware interupt (ISR) - Data Ready
     */
    void publishFromISR(const T& new_data, BaseType_t* pxHigherPriorityTaskWoken);

    /**
     * @brief Check new data from 'last_gen'
     */
    bool updated(uint32_t last_gen) const;

    /**
     * @brief Copy new data to 'out_data' and update 'io_last_gen'
     * @return true if new data which is not read by Subscriber
     */
    bool copy(T& out_data, uint32_t& io_last_gen, uint64_t* out_timestamp_us = nullptr) const;

    /**
     * @brief Register Task receive wake up signal (Task Notification)
     */
    bool registerWatcher(TaskHandle_t task_handle);


    private:
    T data_{};
    uint64_t timestamp_us_ = 0;
    uint32_t generation_ = 0;
    bool published_ = false;

    TaskHandle_t watchers_[MAX_WATCHERS] = {nullptr};
    size_t watcher_count_ = 0;

    mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};


/**
 * @brief Subscription Class
 * @details each Module/Task create a variable OrbSubscription by myself to save trace 'last_gen'
 */
template <typename T, size_t MAX_WATCHERS = 4>
class OrbSubscription
{
    public:
    explicit OrbSubscription(OrbTopic<T, MAX_WATCHERS>& topic) : topic_(topic) {}

    /**
     * @brief Check whether the new data is read ?
     */
    bool updated() const;

    /**
     * @brief Only copy data to 'out_data' if there is a new data from the last read
     * @return true if there is a new data and copy successfully
     */
    bool update(T& out_data);

    /**
     * @brief Always copy new data on Topic
     * @return false if Topic has never published until reset
     */
    bool get(T& out_data);

    /**
     * @brief Current task sleep until Topic has a new data
     * @note Need to call registerCurrentTask() once before while loop(true)
     */
    bool waitForUpdate(T& out_data, TickType_t timeout_ticks);

    /**
     * @brief Register current Task to be watcher of topic
     */
    bool registerCurrentTask();

    /**
     * @brief Get time of news
     */
    uint64_t timestampUs() const;

    private:
    OrbTopic<T, MAX_WATCHERS>& topic_;
    uint32_t last_gen_ = 0;
    uint64_t last_timestamp_us_ = 0;
};

