#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>
#include <boost/asio/io_context.hpp>
#include <thread>

class Exchange {
public:
    Exchange() :
                server("localhost"), 
                vhost("/"), port(5672), 
                exchangeName("devices_exchange"), 
                handler(ioContext), connection(&handler, AMQP::Address(server, port, AMQP::Login("guest", "guest"), vhost)), 
                channel(&connection) {}

    ~Exchange() {
        channel.close();
        connection.close();
    }

    void send_data(const std::string& queueName, const std::string& routingKey, const std::string& messageBody) {
        channel.declareExchange(exchangeName, AMQP::direct);
        channel.declareQueue(queueName);
        channel.bindQueue(exchangeName, queueName, routingKey);
        channel.publish(exchangeName, routingKey, messageBody);
        
    }

    void receive_data(const std::string& queueName, const std::string& routingKey){
        channel.declareExchange(exchangeName, AMQP::direct);
        channel.declareQueue(queueName);
        channel.bindQueue(exchangeName, queueName, routingKey);
        channel.consume(queueName)
            .onReceived([&](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
                handleMessage(message);
                channel.ack(deliveryTag); // Acknowledge the message
            })
            .onSuccess([&]() {
                std::cout << "Consuming started successfully." << std::endl;
            });
        

    }

    void runReceiverInThread(std::string& queueName, std::string& routingKey) {
        std::thread([this, queueName, routingKey]() {
            receive_data(queueName, routingKey);
        }).detach();
    }

    void runIoContext() {
        ioContext.run();
    }

    void runIoInThread(){
        std::thread([this] (){
            runIoContext();
        }).detach();
    }

    
private:
    void handleMessage(const AMQP::Message &message) {
        std::string messageBody(message.body(), message.bodySize());
        std::cout << "Received message: " << messageBody << std::endl;
    }
    std::string server;
    std::string vhost;
    int port;
    std::string exchangeName;
    boost::asio::io_context ioContext;
    AMQP::LibBoostAsioHandler handler;
    AMQP::TcpConnection connection;
    AMQP::TcpChannel channel;
};
