#!/bin/bash

# 检查当前目录下是否存在 build 文件夹
if [ -d "build" ]; then
    # 如果存在，删除 build 文件夹及其内容
    rm -rf build
fi

# 创建 build 文件夹
mkdir build

# 进入 build 文件夹
cd build

# 执行 cmake 命令
cmake ..

# 执行 make 命令
make

# 检查 make 命令的执行结果
if [ $? -eq 0 ]; then
    echo "编译成功！"
else
    echo "编译失败！"
fi