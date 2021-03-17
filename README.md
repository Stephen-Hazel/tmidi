# tmidi
test out midi in straight c++

CMakeLists.txt needs this line tacked on
and ../stv/os.cpp should be linked in

target_link_libraries(tmidi PRIVATE asound)
