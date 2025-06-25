cmake_minimum_required(VERSION 3.15)
project(chat_project C)

set(CMAKE_C_STANDARD 99)

# Подключаем pthread для многопоточности
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Сервер
add_executable(server server.c)
target_link_libraries(server Threads::Threads)

# Клиент
add_executable(client client.c)
target_link_libraries(client Threads::Threads)

# Тесты (если есть, оставим для примера)
add_executable(test test.c)
target_link_libraries(test Threads::Threads)



