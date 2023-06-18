#set(SRC_LIST calluserservice.cc ../user.pb.cc)  #设置需要包含哪些源文件
set(SRC_LIST callfriendservice.cc ../friend.pb.cc)
add_executable(consumer ${SRC_LIST})

target_link_libraries(consumer mprpc protobuf) #链接mprpc框架和protobuf的动态库