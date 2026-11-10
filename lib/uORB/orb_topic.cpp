#include "orb_topic.hpp"


template <typename T, size_t MAX_WATCHERS>
void OrbTopic<T, MAX_WATCHERS>::publish(const T& new_data)
{
    TaskHandle_t watchers_to_notify[MAX_WATCHERS] = {nullptr};
    size_t watcher_count = 0;

    // Spinlock bao ve giua Core 0 va Core 1
    portENTER_CRITICAL(&mux_);
    data_ = new_data;
    timestamp_us_ = esp_timer_get_time();
    generation_++;
    published_ = true;

    // Sao chep danh sach cac Task dang cho ra bien tam de goi Notify ngoai Spinlock
    for (size_t i = 0; i < watcher_count_; ++i)
    {
        watchers_to_notify[i] = watchers_[i];
    }
    watcher_count = watcher_count_;
    portEXIT_CRITICAL(&mux_);

    // Danh thuc cac Task dang cho du lieu moi tu Topic nay
    for (size_t i = 0; i < watcher_count; ++i)
    {
        if (watchers_to_notify[i] != nullptr)
        {
            xTaskNotifyGive(watchers_to_notify[i]);
        }
    }
}

template <typename T, size_t MAX_WATCHERS>
void OrbTopic<T, MAX_WATCHERS>::publishFromISR(const T& new_data, BaseType_t* pxHigherPriorityTaskWoken)
{
    TaskHandle_t watchers_to_notify[MAX_WATCHERS] = {nullptr};
    size_t watcher_count = 0;

    portENTER_CRITICAL_ISR(&mux_);
    data_ = new_data;
    timestamp_us_ = esp_timer_get_time();
    generation_++;
    published_ = true;

    for (size_t i = 0; i < watcher_count_; ++i)
    {
        watchers_to_notify[i] = watchers_[i];
    }

    watcher_count = watcher_count_;
    portEXIT_CRITICAL_ISR(&mux_);

    for (size_t i = 0; i < watcher_count; ++i)
    {
        if (watchers_to_notify[i] != nullptr)
        {
            vTaskNotifyGiveFromISR(watchers_to_notify[i], pxHigherPriorityTaskWoken);
        }
    }
}


template <typename T, size_t MAX_WATCHERS>
bool OrbTopic<T, MAX_WATCHERS>::updated(uint32_t last_gen) const
{
    portENTER_CRITICAL(&mux_);
    bool is_new = published_ && (generation_ != last_gen);
    portEXIT_CRITICAL(&mux_);
    return is_new;
}

template <typename T, size_t MAX_WATCHERS>
bool OrbTopic<T, MAX_WATCHERS>::copy(T& out_data, uint32_t& io_last_gen, uint64_t* out_timestamp_us) const
{
    portENTER_CRITICAL(&mux_);
    if (!published_)
    {
        portEXIT_CRITICAL(&mux_);
        return false;
    }

    bool is_new = (generation_ != io_last_gen);
    out_data = data_;
    io_last_gen = generation_;
    if (out_timestamp_us != nullptr)
    {
        *out_timestamp_us = timestamp_us_;
    }
    portEXIT_CRITICAL(&mux_);
    return is_new;
}

template <typename T, size_t MAX_WATCHERS>
bool OrbTopic<T, MAX_WATCHERS>::registerWatcher(TaskHandle_t task_handle)
{
    if (task_handle == nullptr) return false;

    bool success = false;
    portENTER_CRITICAL(&mux_);

    // Kiem tra xem task da dang ki truoc do chua
    for (size_t i = 0; i < watcher_count_; ++i)
    {
        if (watchers_[i] == task_handle)
        {
            portEXIT_CRITICAL(&mux_);
            return true;
        }
    }

    // Them vao danh sach neu con cho trong
    if (watcher_count_ < MAX_WATCHERS)
    {
        watchers_[watcher_count_++] = task_handle;
        success = true;
    }
    portEXIT_CRITICAL(&mux_);
    return success;
}

template <typename T, size_t MAX_WATCHERS>
bool OrbSubscription<T, MAX_WATCHERS>::updated() const
{
    return topic_.updated(last_gen_);
}


template <typename T, size_t MAX_WATCHERS>
bool OrbSubscription<T, MAX_WATCHERS>::update(T& out_data)
{
    if (topic_.updated(last_gen_))
    {
        return topic_.copy(out_data, last_gen_, &last_timestamp_us_);
    }
    return false;
}

template <typename T, size_t MAX_WATCHERS>
bool OrbSubscription<T, MAX_WATCHERS>::get(T& out_data)
{
    return topic_.copy(out_data, last_gen_, &last_timestamp_us_);
}


template <typename T, size_t MAX_WATCHERS>
bool OrbSubscription<T, MAX_WATCHERS>::waitForUpdate(T& out_data, TickType_t timeout_ticks)
{
    if (ulTaskNotifyTake(pdTRUE, timeout_ticks) > 0)
    {
        return get(out_data);
    }
    return false;
}



template <typename T, size_t MAX_WATCHERS>
bool OrbSubscription<T, MAX_WATCHERS>::registerCurrentTask()
{
    return topic_.registerWatcher(xTaskGetCurrentTaskHandle());
}


template <typename T, size_t MAX_WATCHERS>
uint64_t OrbSubscription<T, MAX_WATCHERS>::timestampUs() const
{
    return last_timestamp_us_;
}