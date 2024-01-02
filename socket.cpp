#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <memory>
#include <asio.hpp>


using namespace std;
using namespace asio;
using namespace asio::ip;

class TCPSession : public enable_shared_from_this<TCPSession> {
public:
    TCPSession(tcp::socket socket, map<tcp::endpoint, shared_ptr<TCPSession>>& sessions)
        : socket_(move(socket)), sessions_(sessions) {
        // Store the client's endpoint information
        endpoint_ = socket_.remote_endpoint();
    }

    void start() {
        if (sessions_.size() < 1000){
            std::cout << sessions_.size() + 1 << std::endl;
            sessions_.emplace(endpoint_, shared_from_this());
        }
        else{
            cout << "not allowed" << endl;
            socket_.close();

        }
        doRead();
    }

    void sendMessage(const string& message) {
        // Post the message to the socket's executor
        post(socket_.get_executor(),
             [self = shared_from_this(), message]() {
                 async_write(self->socket_, buffer(message),
                             [](const error_code& ec, size_t /*length*/) {
                                 if (ec) {
                                     cerr << "Write error: " << ec.message() << endl;
                                 }
                             });
             });
    }

private:
    void doRead() {
        auto self(shared_from_this());
        socket_.async_read_some(buffer(data_),
            [this, self](const error_code& ec, size_t length) {
                if (!ec) {
                    onMessage(data_.data(), length);
                    doRead();
                } else {
                    onClose(ec);
                }
            }
        );
    }

    void onMessage(const char* data, size_t length) {
        cout << "Received message from " << endpoint_ << ": " << string(data, length) << endl;

        // Implement your logic for processing received data here

        // For example, echo the message back to the client
        sendMessage("Echo: " + string(data, length));
    }

    void onClose(const error_code& ec) {
        cerr << "Closed at " << endpoint_ << endl;
        sessions_.erase(endpoint_);
        socket_.close();
        
    }

    tcp::socket socket_;
    tcp::endpoint endpoint_;
    array<char, 1024> data_;
    map<tcp::endpoint, shared_ptr<TCPSession>>& sessions_;
};

class TCPServer {
public:
    TCPServer(io_context& ioContext, short port)
        : acceptor_(ioContext, tcp::endpoint(tcp::v4(), port)),
          socket_(ioContext) {
        startAccept();
        startMessageThread();  // Start the additional thread
    }

private:
    void startAccept() {
        acceptor_.async_accept(socket_, [this](const error_code& ec) {
            if (!ec) {
                make_shared<TCPSession>(move(socket_), sessions_)->start();
            }

            startAccept();
        });
    }

    void startMessageThread() {
        // Start a thread that sends a message every 2 seconds
        std::thread([this]() {
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                broadcastMessage("Hello from server!");
            }
        }).detach();
    }

    void broadcastMessage(const string& message) {
        // Broadcast the message to all connected clients
        for (const auto& pair : sessions_) {
            const auto& session = pair.second;
            session->sendMessage(message);
        }
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    map<tcp::endpoint, shared_ptr<TCPSession>> sessions_;
};
