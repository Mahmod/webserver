#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <map>

std::string get_file_content(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
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

    return "./data" + path;  // Prepending the data folder to construct the relative file path
}

std::string get_content_type(const std::string& file_path) {
    std::map<std::string, std::string> content_type_map = {
        {".html", "text/html"},
        {".js", "application/javascript"},
        {".css", "text/css"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},
        {".eot", "application/vnd.ms-fontobject"},
        // Add more content types as needed
    };

    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string extension = file_path.substr(dot_pos);
        if (content_type_map.find(extension) != content_type_map.end()) {
            return content_type_map[extension];
        }
    }

    return "application/octet-stream";  // Default binary data MIME type
}

int main() {
    // Creating a socket
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[4096] = {0};  // Increased buffer size to handle larger requests

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

        long valread = read(new_socket, buffer, 4096);
        if(valread < 0) {
            std::cerr << "Read failed" << std::endl;
            return -1;
        }

        std::cout << "Request:\n" << buffer << std::endl;
        
        std::string file_path = get_requested_file_path(buffer);
        std::string file_content = get_file_content(file_path);
        std::string content_type = get_content_type(file_path);
        
        if (file_content.empty()) {
            file_content = get_file_content("./data/404.html");  // Custom HTML error page
            if (file_content.empty()) {
                file_content = "<html><body><h1>404 Not Found</h1></body></html>";  // Default error message if custom page is also not found
            }
            std::string not_found_response = "HTTP/1.1 404 Not Found\nContent-Type: " + content_type + "\nContent-Length: " + std::to_string(file_content.length()) + "\n\n" + file_content;
            send(new_socket, not_found_response.c_str(), not_found_response.length(), 0);
        } else {
            std::string response = "HTTP/1.1 200 OK\nContent-Type: " + content_type + "\nContent-Length: " + std::to_string(file_content.length()) + "\n\n" + file_content;
            send(new_socket, response.c_str(), response.length(), 0);
        }

        std::cout << "Response sent\n" << std::endl;

        close(new_socket);
    }

    return 0;
}
