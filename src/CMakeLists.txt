aux_source_directory(. SRC_LIST) #当前目录下的所有文件编译成 .SRC_LIST
add_library(mprpc ${SRC_LIST}) #生成一个动态库
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt) #添加依赖,muduo_net是核心，先添加muduo_net，再添加muduo_base zookeeper多线程版本