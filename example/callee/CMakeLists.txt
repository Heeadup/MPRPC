#set(SRC_LIST userservice.cc ../user.pb.cc)  #设置需要包含哪些源文件
set(SRC_LIST friendservice.cc ../friend.pb.cc) 
add_executable(provider ${SRC_LIST})

target_link_libraries(provider mprpc protobuf) #链接mprpc框架和protobuf的动态库