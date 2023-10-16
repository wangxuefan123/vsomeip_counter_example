#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
#include <csignal>
#endif
#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
//#include <atomic>
#include <vsomeip/vsomeip.hpp>

#include "sample-ids.hpp"

class CounterService
{
public:
    CounterService() : app_(vsomeip::runtime::get()->create_application("CounterService")),
                       is_registered_(false),
                       counter_(0),
                       is_offered_(false),
                       running_(true),
                       stop_(false),
                       //request_("s"),
                       // offer_thread_(std::bind(&CounterService::run, this)),
                       check_thread_(std::bind(&CounterService::check_request, this)),
                       count_thread_(std::bind(&CounterService::counting, this)),
                       notify_thread_(std::bind(&CounterService::notify, this))
    {
    }

    void on_state(vsomeip::state_type_e _state)
    {
        std::cout << "Application " << app_->get_name() << " is "
                  << (_state == vsomeip::state_type_e::ST_REGISTERED ? "registered." : "deregistered.") << std::endl;

        if (_state == vsomeip::state_type_e::ST_REGISTERED)
        {
            if (!is_registered_)
            {
                is_registered_ = true;
            }
        }
        else
        {
            is_registered_ = false;
        }
    }

    void check_request()
    {
        std::cout << "check_request() CALLED" << std::endl;
        while(running_)
        {
            // if(payload_str =="au"){
            //     stop_ = false;
            //     request_ = payload_str;
            // }
            // else if(payload_str =="ad")
            // {
            //     stop_ = false;
            //     request_ = payload_str;
            // }
            // else if(payload_str =="s")
            // {
            //     stop_ = true;
            // }
            if(request_ == "s"){
                stop_ = true;
                break;
            }
            else{
                stop_ = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void counting()
    {
        std::cout << "counting() CALLED" << std::endl;
        //std::lock_guard<std::mutex> its_lock(counter_mutex_);
        while(running_)
        {
            if(request_ == "au")
            {
                for (; counter_ <= 20 && !stop_; ++counter_) {
                    std::cout << "SERVICE: Incremented counter to " << counter_ << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
            else if (request_ == "ad")
            {
                for (; counter_ >= 0 && !stop_; --counter_) {
                    std::cout << "SERVICE: Decremented counter to " << counter_ << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        }
    }

    void notify()
    {
        while (running_)
        {
            std::unique_lock<std::mutex> its_lock(notify_mutex_);
            while (not is_offered_ && running_)
            {
                std::cout << "[INFO] Wait main thread enter blocked state !" << std::endl;
                notify_condition_.wait(its_lock);
            }

            {
                std::lock_guard<std::mutex> its_lock(counter_mutex_);
                if (counter_ == 8)
                {
                    std::cout << "[INFO] EVENT triggered! Counter = 8, NOTIFY subscribers!" << std::endl;
                    std::string notify_msg = "~EVENT EIGHT~";
                    std::vector<vsomeip::byte_t> notify_payload_data(notify_msg.begin(), notify_msg.end());
                    std::shared_ptr<vsomeip::payload> notify_payload;

                    notify_payload = vsomeip::runtime::get()->create_payload();
                    notify_payload->set_data(notify_payload_data);
                    app_->notify(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_EVENT_ID, notify_payload);
                }
            }
            // Event check cycle
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void on_message(const std::shared_ptr<vsomeip::message> &_request)
    {
        std::lock_guard<std::mutex> its_lock(counter_mutex_);
        std::cout << "ON_MESSAGE CALLED" << std::endl;
        std::shared_ptr<vsomeip::payload> its_payload = _request->get_payload();
        vsomeip::length_t l = its_payload->get_length();

        // Get payload as a string
        std::string payload_str;
        for (vsomeip::length_t i = 0; i < l; i++)
        {
            payload_str += static_cast<char>(*(its_payload->get_data() + i));
        }

        if (payload_str == "u")
        {
            // Increment the counter
            counter_++;
            std::cout << "SERVICE: Incremented counter to " << counter_ << std::endl;
        }
        else if (payload_str == "d")
        {
            // Decrement the counter (with a check to ensure it doesn't go negative)
            if (counter_ > 0)
            {
                counter_--;
                std::cout << "SERVICE: Decremented counter to " << counter_ << std::endl;
            }
            else
            {
                std::cout << "SERVICE: Counter is already at 0, cannot decrement further." << std::endl;
            }
        }
        else if (payload_str =="au" || payload_str =="ad"){
            stop_ = false;
            //stop_.store(false);
            request_ = payload_str;
        }
        else if(payload_str =="s")
        {
            stop_ = true;
            //stop_.store(true);
            request_ = payload_str;
        }
        else
        {
            std::cout << "SERVICE: Received an unsupported message: " << payload_str << std::endl;
        }

        std::shared_ptr<vsomeip::message> its_response = vsomeip::runtime::get()->create_response(_request);
        its_payload = vsomeip::runtime::get()->create_payload();
        const vsomeip::byte_t its_payload_data[] = {0x10};
        // auto its_payload_data = get_payload_data(counter_);

        its_payload->set_data(its_payload_data, sizeof(its_payload_data));
        its_response->set_payload(its_payload);
        app_->send(its_response);
    }

    bool init()
    {
        std::lock_guard<std::mutex> its_lock(notify_mutex_);
        // init the application
        if (!app_->init())
        {
            std::cerr << "Couldn't initialize application" << std::endl;
            return false;
        }
        app_->register_state_handler(
            std::bind(&CounterService::on_state, this,
                      std::placeholders::_1));

        app_->register_message_handler(
            vsomeip::ANY_SERVICE, SAMPLE_INSTANCE_ID, vsomeip::ANY_METHOD, ////
            std::bind(&CounterService::on_message, this,
                      std::placeholders::_1));

        // app_->register_message_handler(
        //         SAMPLE_SERVICE_ID,
        //         SAMPLE_INSTANCE_ID,
        //         SAMPLE_SET_METHOD_ID,
        //         std::bind(&CounterService::on_get, this,
        //                   std::placeholders::_1));

        // app_->register_message_handler(
        //         SAMPLE_SERVICE_ID,
        //         SAMPLE_INSTANCE_ID,
        //         SAMPLE_SET_METHOD_ID,
        //         std::bind(&CounterService::on_set, this,
        //                   std::placeholders::_1));

        app_->offer_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);

        std::set<vsomeip::eventgroup_t> its_groups;
        its_groups.insert(SAMPLE_EVENTGROUP_ID);
        app_->offer_event(
            SAMPLE_SERVICE_ID,
            SAMPLE_INSTANCE_ID,
            SAMPLE_EVENT_ID,
            its_groups,
            vsomeip::event_type_e::ET_EVENT, std::chrono::milliseconds::zero(),
            false, true, nullptr, vsomeip::reliability_type_e::RT_UNKNOWN);
        // {
        //     std::lock_guard<std::mutex> its_lock(payload_mutex_);
        //     payload_ = vsomeip::runtime::get()->create_payload();
        // }

        // blocked_ = true;
        is_offered_ = true;
        notify_condition_.notify_one();
        return true;
    }

    void start() { app_->start(); }

    // void on_get(const std::shared_ptr<vsomeip::message> &_message) {
    //     std::shared_ptr<vsomeip::message> its_response
    //         = vsomeip::runtime::get()->create_response(_message);
    //     {
    //         std::lock_guard<std::mutex> its_lock(payload_mutex_);
    //         its_response->set_payload(payload_);
    //     }
    //     app_->send(its_response);
    // }

    // void on_set(const std::shared_ptr<vsomeip::message> &_message) {
    //     std::shared_ptr<vsomeip::message> its_response
    //         = vsomeip::runtime::get()->create_response(_message);
    //     {
    //         std::lock_guard<std::mutex> its_lock(payload_mutex_);
    //         payload_ = _message->get_payload();
    //         its_response->set_payload(payload_);
    //     }

    //     app_->send(its_response);
    //     app_->notify(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID,
    //                  SAMPLE_EVENT_ID, payload_);
    // }

private:
    std::shared_ptr<vsomeip::application> app_;
    bool is_registered_;
    unsigned int counter_;

    std::mutex counter_mutex_;
    // std::condition_variable condition_;
    // bool blocked_;
    bool running_;
    //std::atomic<bool> stop_;
    bool stop_;
    std::string request_;

    std::mutex notify_mutex_;
    std::condition_variable notify_condition_;
    bool is_offered_;

    // std::mutex payload_mutex_;
    // std::shared_ptr<vsomeip::payload> payload_;

    std::thread check_thread_;
    std::thread count_thread_;
    std::thread notify_thread_;
};

int main()
{
    // auto service = CounterService();
    CounterService service;

    if (!service.init())
    {
        return EXIT_FAILURE;
    };
    service.start();
    // if (service.init()) {
    //     // Start the offer_thread_ separately
    //     std::thread offer_thread([&service] { service.run(); });
    //     // Wait for the offer_thread_ to finish
    //     offer_thread.join();
    // }
    return 0;
}
