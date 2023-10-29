#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

std::string get_file_content(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << file_path << std::endl;
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return content;
}

std::string get_requested_file_path(const std::string& request) {
    std::istringstream request_stream(request);
    std::string method, path, protocol;
    request_stream >> method >> path >> protocol;
    std::cout << "Method: " << method << std::endl;
    std::cout << "Path: " << path << std::endl;
    std::cout << "Protocol: " << protocol << std::endl;
    
    if (method != "GET") {
        return "";
    }
    
    if (path == "/") {
        path = "/index.html";
    }

    // Preventing directory traversal attacks by removing ".." from path
    size_t pos;
    while ((pos = path.find("..")) != std::string::npos) {
        path.erase(pos, 2);
    }

    return "." + path;  // Prepending a dot to construct the relative file path
}

int main() {
    // Creating a socket
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket failed" << std::endl;
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "Setsockopt failed" << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }

    while (true) {
        std::cout << "Waiting for connections..." << std::endl;

        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept failed" << std::endl;
            return -1;
        }

        long valread = read(new_socket, buffer, 1024);
        if(valread < 0) {
            std::cerr << "Read failed" << std::endl;
            return -1;
        }

        std::cout << "Request:\n" << buffer << std::endl;
        
        std::string file_path = get_requested_file_path(buffer);
        std::string file_content = get_file_content(file_path);
        
        if (file_content.empty()) {
            std::string not_found_response = "HTTP/1.1 404 Not Found\nContent-Type: text/plain\nContent-Length: 9\n\nNot Found";
            send(new_socket, not_found_response.c_str(), not_found_response.length(), 0);
        } else {
            std::string response = "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " + std::to_string(file_content.length()) + "\n\n" + file_content;
            send(new_socket, response.c_str(), response.length(), 0);
        }

        std::cout << "Response sent\n" << std::endl;

        close(new_socket);
    }

    return 0;
}
