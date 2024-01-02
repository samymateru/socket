#include "iostream"
#include "communication.cpp"
#include "socket.cpp"
int main() {
    try {
        Exchange exchange;
        std:string queue = "devices_queue";
        std::string routing_key = "devices_routing_key";
        // exchange.runReceiverInThread(queue, routing_key );
        exchange.send_data(queue, routing_key,"samwel");
        exchange.runIoInThread();
        io_context ioContext;
        TCPServer server(ioContext, 8080);

        // Run the IO context on multiple threads
        vector<std::thread> threads;
        for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
            threads.emplace_back([&ioContext]() {
                ioContext.run();
            });
        }

        // Wait for all threads to finish
        for (auto& thread : threads) {
            thread.join();
        }
    } catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }

    return 0;
}
