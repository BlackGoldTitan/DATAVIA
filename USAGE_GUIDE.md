# CRCRECOVER 使用指南

## 问题解决：磁盘访问权限错误

如果您遇到 "Cannot access disk, error code: 2" 错误，请按照以下步骤解决：

### 1. 管理员权限运行

在Windows系统中，访问物理磁盘需要管理员权限：

#### 方法一：命令行方式
```cmd
# 以管理员身份运行命令提示符
# 然后运行程序
CRCRECOVER.exe generate C: 0 100 checksums.dat
```

#### 方法二：右键菜单
- 右键点击程序文件
- 选择"以管理员身份运行"

#### 方法三：PowerShell
```powershell
# 以管理员身份运行PowerShell
Start-Process "CRCRECOVER.exe" -ArgumentList "generate C: 0 100 checksums.dat" -Verb RunAs
```

### 2. 正确的磁盘路径格式

#### 逻辑分区（推荐用于测试）
```bash
CRCRECOVER generate C: 0 100 checksums.dat
CRCRECOVER generate D: 0 100 checksums.dat
```

#### 物理磁盘（需要管理员权限）
```bash
CRCRECOVER generate \\.\PhysicalDrive0 0 100 checksums.dat
CRCRECOVER generate \\.\PhysicalDrive1 0 100 checksums.dat
```

### 3. 测试建议

#### 小规模测试（避免权限问题）
```bash
# 只测试少量扇区
CRCRECOVER generate C: 0 10 checksums.dat
CRCRECOVER verify C: checksums.dat
```

#### 验证磁盘可访问性
```bash
# 使用磁盘列表功能检查可用磁盘
CRCRECOVER list
```

### 4. 图形界面使用

1. 以管理员身份运行 `CRCRECOVER_GUI.exe`
2. 在界面中选择要操作的磁盘
3. 设置起始扇区和扇区数量
4. 点击相应按钮执行操作

## 功能说明

### 生成校验数据
```bash
CRCRECOVER generate <磁盘路径> <起始扇区> <扇区数量> <输出文件>
```
示例：
```bash
CRCRECOVER generate C: 0 1000 checksums.dat
```

### 验证数据完整性
```bash
CRCRECOVER verify <磁盘路径> <校验文件>
```
示例：
```bash
CRCRECOVER verify C: checksums.dat
```

### 修复损坏数据
```bash
CRCRECOVER repair <磁盘路径> <校验文件> [备份磁盘路径]
```
示例：
```bash
CRCRECOVER repair C: checksums.dat D:
```

## 性能优化特性

### 并行处理
- 自动使用多线程并行处理
- 支持2-8个线程同时工作
- 显著提升处理速度（2-4倍）

### 批量读取
- 自动启用批量I/O操作
- 减少磁盘访问开销
- 优化内存使用

### 操作取消
- 支持随时取消长时间操作
- 安全的线程终止机制

## 注意事项

1. **数据安全**: 修复操作会直接写入磁盘，请确保有有效备份
2. **权限要求**: 物理磁盘操作需要管理员权限
3. **磁盘锁定**: 操作期间磁盘可能被锁定，避免其他程序访问
4. **性能影响**: 大量扇区操作可能耗时较长

## 故障排除

### 常见错误及解决方案

#### 错误: "Cannot access disk, error code: 2"
- **原因**: 磁盘路径不存在或权限不足
- **解决**: 以管理员身份运行程序，检查磁盘路径

#### 错误: "Access denied"
- **原因**: 权限不足
- **解决**: 以管理员身份运行程序

#### 错误: "Disk is in use by another process"
- **原因**: 磁盘被其他程序占用
- **解决**: 关闭其他可能使用该磁盘的程序

#### 错误: "Invalid disk path"
- **原因**: 磁盘路径格式错误
- **解决**: 使用正确的磁盘路径格式

## 技术支持

如果问题仍然存在，请检查：
1. 操作系统版本和权限设置
2. 磁盘状态和文件系统类型
3. 程序运行环境（管理员权限）
4. 磁盘路径格式是否正确

程序已针对Windows系统优化，提供了详细的错误信息和用户友好的提示。
