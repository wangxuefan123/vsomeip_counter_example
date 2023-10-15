#include <iomanip>
#include <iostream>
#include <sstream>

#include <condition_variable>
#include <thread>

#include <vsomeip/vsomeip.hpp>

#include "sample-ids.hpp"

std::shared_ptr< vsomeip::application > app;
std::mutex mutex;
std::condition_variable condition;

void run() {
    std::unique_lock<std::mutex> its_lock(mutex);
    condition.wait(its_lock);

    // Prompt the user for input
    std::string user_input;
    std::cout << "Enter 'u'(up)/'d'(down) or 'au'(autoup)/'ad'(autodown) to send a request to the server: ";

    while(true){
        std::cin >> user_input;

        // Check if the user input is valid ("up" or "down")
        if (user_input == "u" || user_input == "d" || user_input == "au" || user_input == "ad" || user_input == "s") {
            std::shared_ptr<vsomeip::message> request = vsomeip::runtime::get()->create_request();
            request->set_service(SAMPLE_SERVICE_ID);
            request->set_instance(SAMPLE_INSTANCE_ID);
            request->set_method(SAMPLE_METHOD_ID);

            //std::shared_ptr<vsomeip::payload> request_payload = vsomeip::runtime::get()->create_payload();
            // Convert user_input string to byte array
            std::vector<vsomeip::byte_t> request_data(user_input.begin(), user_input.end());
            
            std::shared_ptr<vsomeip::payload> request_payload = vsomeip::runtime::get()->create_payload();
            request_payload->set_data(request_data);
            //request_payload->set_data(user_input.c_str(), user_input.length());
            request->set_payload(request_payload);

            app->send(request);
            std::cout << "CLIENT: Sent request to server: " << user_input << std::endl;
        } else {
            std::cout << "CLIENT: Invalid input. Please enter 'up' or 'down'." << std::endl;
        }
    }
}

void on_message(const std::shared_ptr<vsomeip::message> &_response) {

  std::shared_ptr<vsomeip::payload> its_payload = _response->get_payload();
  vsomeip::length_t l = its_payload->get_length();

  // Get payload
  std::stringstream ss;
  for (vsomeip::length_t i=0; i<l; i++) {
     ss << std::setw(2) << std::setfill('0') << std::hex
        << (int)*(its_payload->get_data()+i) << " ";
  }

  std::cout << "CLIENT: Received message with Client/Session ["
      << std::setw(4) << std::setfill('0') << std::hex << _response->get_client() << "/"
      << std::setw(4) << std::setfill('0') << std::hex << _response->get_session() << "] "
      << ss.str() << std::endl;
}

void on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available) {
    std::cout << "CLIENT: Service ["
            << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
            << "] is "
            << (_is_available ? "available." : "NOT available.")
            << std::endl;
    condition.notify_one();
}

int main() {

    app = vsomeip::runtime::get()->create_application("Hello");
    app->init();
    app->register_availability_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, on_availability);
    app->request_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, on_message);
    std::thread sender(run);
    app->start();
}