# CRCRECOVER - 完整项目总结

## 项目概述

CRCRECOVER 是一个用C++实现的磁盘数据完整性校验和修复工具，支持扇区级和文件系统级的数据完整性检查和修复。

## 核心功能

### 1. 扇区级数据完整性检查
- **生成校验和**: 为指定硬盘扇区生成CRC32校验和
- **验证完整性**: 使用校验和文件验证扇区数据完整性
- **数据修复**: 从备份磁盘修复损坏的扇区数据
- **物理磁盘支持**: 支持物理磁盘和逻辑分区

### 2. 文件系统级数据完整性检查
- **文件校验**: 为单个文件生成CRC32校验和
- **目录校验**: 递归扫描目录，为所有文件生成校验和
- **分区校验**: 对整个分区进行完整性检查
- **文件修复**: 从备份文件或目录修复损坏的文件

### 3. 增强功能
- **操作取消**: 支持随时取消长时间运行的操作
- **并行处理**: 多线程并行处理，显著提高性能
- **校验文件格式**: 标准化的二进制校验文件格式
- **进度回调**: 实时进度反馈和状态更新

## 项目结构

### 核心类
- `DiskSectorCRC`: 基础扇区校验类
- `EnhancedDiskSectorCRC`: 增强扇区校验类（支持取消和并行）
- `FileSystemCRC`: 文件系统校验类

### 主要文件
```
CRCRECOVER/
├── main.cpp                    # 命令行主程序
├── main_gui.cpp               # GUI主程序
├── DiskSectorCRC.h/cpp        # 基础扇区校验
├── EnhancedDiskSectorCRC.h/cpp # 增强扇区校验
├── FileSystemCRC.h/cpp        # 文件系统校验
├── GUIWindow.h/cpp            # GUI界面
├── CMakeLists.txt             # 构建配置
├── test_*.cpp                 # 测试程序
└── *.md                       # 文档
```

## 技术特性

### 1. 多平台支持
- 使用标准C++17和STL
- 跨平台文件系统操作
- 条件编译支持Windows特定功能

### 2. 性能优化
- 多线程并行处理
- 内存高效使用
- 批量I/O操作

### 3. 错误处理
- 完善的错误报告机制
- 异常安全设计
- 资源自动管理

### 4. 用户界面
- 命令行界面（CLI）
- 图形用户界面（GUI）
- 实时进度显示

## 使用示例

### 扇区级操作
```bash
# 生成扇区校验和
CRCRECOVER generate C: 0 1000 checksums.dat

# 验证数据完整性
CRCRECOVER verify C: checksums.dat

# 修复损坏数据
CRCRECOVER repair C: checksums.dat D:
```

### 文件系统级操作
```cpp
FileSystemCRC fsCRC;

// 生成目录校验和
DirectoryChecksum checksum;
fsCRC.generateDirectoryChecksums("C:/MyFolder", checksum);

// 验证完整性
bool valid = fsCRC.verifyDirectoryIntegrity(checksum);

// 从备份修复
fsCRC.repairDirectoryFromBackup(checksum, "D:/Backup");
```

## 编译和运行

### 编译
```bash
cmake -B build -S .
cmake --build build --config Release
```

### 运行
```bash
# 命令行版本
build\CRCRECOVER.exe

# GUI版本  
build\CRCRECOVER_GUI.exe
```

## 测试程序

项目包含多个测试程序：
- `test_crc32.cpp`: CRC32算法测试
- `test_enhanced.cpp`: 增强功能测试
- `test_filesystem.cpp`: 文件系统功能测试

## 安全注意事项

1. **权限要求**: 访问物理磁盘需要管理员权限
2. **数据备份**: 修复操作前建议备份重要数据
3. **操作验证**: 修复后验证数据完整性
4. **磁盘锁定**: 操作期间磁盘可能被锁定

## 扩展性

项目设计具有良好的扩展性：
- 可添加新的校验算法
- 支持更多文件系统类型
- 可集成到其他应用程序
- 支持插件架构

## 总结

CRCRECOVER 提供了一个完整的磁盘数据完整性检查和修复解决方案，结合了扇区级和文件系统级的保护机制。通过并行处理、操作取消和实时进度反馈等增强功能，为用户提供了高效、可靠的数据保护工具。

项目代码结构清晰，文档完善，易于维护和扩展，适合用于数据恢复、系统维护和数据完整性验证等场景。
