# AGENTS.md

## 最高优先级沟通与输出约定（非 CODEX 可编辑区，禁止修改）

下面你的每个回答都要开头带有“喵喵”，用简体中文详细回答，并且要联系上下文。编码一律为UTF-8无BOM，尾行序列为LF，修改文件的时候要注意不要导致文件乱码。
请使用第一性原理思考。你不能总是假设我非常清楚自己想要什么和该怎么得到。请保持审慎，从原始需求和问题出发，如果动机和目标不清晰，停下来和我讨论。如果目标清晰但是路径不是最短，告诉我，并且建议更好的办法请使用第一性原理思考。你不能总是假设我非常清楚自己想要什么和该怎么得到。请保持审慎，从原始需求和问题出发，如果动机和目标不清晰，停下来和我讨论。如果目标清晰但是路径不是最短，告诉我，并且建议更好的办法

## 工程信息沉淀约定（非 CODEX 可编辑区，禁止修改）

你在工作中,遇到可能会在之后需要到的内容需要将他用中文写入AGENTS.md,编码一律为UTF-8无BOM，尾行序列为LF，修改文件的时候要注意不要导致文件乱码,方便之后更好的理解工程。

> 约束补充：新增/更新“之后可能会用到的内容”时，一律写入下方「CODEX 可编辑区」，不要改动本文件其他位置。

## 修改权限边界（非 CODEX 可编辑区，禁止修改）

- **只有**「CODEX 可编辑区」允许 Codex 修改（包括增删改任何文字、格式、链接、列表等）。
- **除「CODEX 可编辑区」外**，Codex 不得改动 `AGENTS.md` 的任何区域（包括但不限于：标题、标点、空行、顺序、措辞、Markdown 结构）。
- 如果 Codex 认为“非可编辑区”必须调整：只能在任务输出里**提出建议**，由人类维护者手工修改；Codex 不得直接改文件。

---

## 编译文件目录树（此处仅允许 Codex 收到指令修改）

## CODEX 可编辑区（仅此处允许 Codex 修改）
<!-- CODEX_EDITABLE_BEGIN -->
<!--
这里专门留给 Codex 维护工程上下文，建议按块追加，便于长期演进，例如：

### 常用命令
- 构建：
- 单测：
- 集成测试：
- 烧录/下载：
- 仿真/调试：

### 目录结构速览
- src/ :
- docs/ :
- tools/ :

### 关键约束/踩坑记录
- （示例）某芯片版本差异：
- （示例）某编译器/链接脚本注意事项：

### 待办 & 近期变更
- TODO：
- CHANGELOG（简版）：
-->

### 关键约束/踩坑记录

- `GSV6715_INT` 当前绑定在 `PD14`，CubeMX 配置为 `GPIO_MODE_IT_RISING_FALLING`，对应中断号为 `EXTI15_10_IRQn`。
- 与 `GSV6715_INT` 相关的自动生成代码落点分别在 `project/Src/main.c` 的 `MX_GPIO_Init()`（NVIC 优先级与使能）和 `project/Src/stm32h7xx_it.c` 的 `EXTI15_10_IRQHandler()`（调用 `HAL_GPIO_EXTI_IRQHandler(GSV6715_INT_Pin)`）。
- 该工程当前使用的 GPIO EXTI HAL 回调入口是 `HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)`；中断链路为 `EXTI15_10_IRQHandler()` -> `HAL_GPIO_EXTI_IRQHandler(GSV6715_INT_Pin)` -> `HAL_GPIO_EXTI_Callback(GPIO_Pin)`，用户逻辑应放在用户文件中重写该弱符号，而不是直接修改 HAL 驱动源码。
- `project/MDK-ARM/project.uvprojx` 重新生成后容易出现大面积 XML 自闭合标签/格式化差异，并伴随 trailing whitespace；提交前建议单独确认是否存在真实工程配置变更。
- `project/Drivers/Gsv650x/userapp/stm32f030c8_gsv6505_gsv_demo/apps/av_user_config_input.h` 当前开启了 `AvEnableUartInput=1`，而 `av_uart_cmd.c` 的 `ProcessChar()` 会在每次 `AvHalUartGetByte()` 返回成功后立刻把收到的字节回显到输出通道。
- 若 RTT 模式下 `project/Drivers/Gsv650x/userbsp/keil/stm32f070cb_gsv6505_gsv_demo/bsp.c` 的 `BspUartGetByte()` 只是空实现却返回 `AvOk`，`ProcessChar()` 会在主循环中持续回显 `0x00`；现象是 RTT 终端几乎看不到文本，但接收端缓存/内存占用持续增长。
- RTT 模式下若需要命令输入，`BspUartGetByte()` 应通过 `SEGGER_RTT_Read(0U, data, 1U)` 从 down-buffer 取字节；若不需要输入，应关闭 `AvEnableUartInput` 或确保无数据时返回 `AvError`。
- 工程 RTT 配置文件位于 `Middlewares/ThirdParty/SEGGER/Config/SEGGER_RTT_Conf.h`，当前 `BUFFER_SIZE_UP=10240`、`BUFFER_SIZE_DOWN=16`、`SEGGER_RTT_PRINTF_BUFFER_SIZE=255`，默认模式为 `SEGGER_RTT_MODE_NO_BLOCK_SKIP`。
- 现有大量 GSV 日志最终会经 `project/Drivers/Gsv650x/uapi/uapi.c` 的 `AvUapiOuputDbgMsg()` 聚合成最多 128 字节字符串后一次性调用 `AvUartTxByte()`；在 `bsp.c` 的 RTT 路径下，这会落到 `SEGGER_RTT_Write(0U, data, size)`。
- 当前 `project/Drivers/Gsv650x/userapp/stm32f030c8_gsv6505_gsv_demo/apps/av_user_config_input.h` 同时开启了 `AvEnableDebugMessage=1` 和 `AvEnableDebugFsm=1`，会放大启动阶段和状态机切换阶段的 RTT 峰值日志量；若 RTT“显示不完”，应优先考虑降低日志源头和调整 RTT 非阻塞策略，而不是直接改成阻塞模式。
- `Middlewares/ThirdParty/SEGGER/RTT/SEGGER_RTT.h` 已内置 ANSI 颜色控制宏，例如 `RTT_CTRL_TEXT_GREEN` 和 `RTT_CTRL_RESET`；在 RTT Viewer 支持 ANSI 控制码的前提下，可直接包裹 `SEGGER_RTT_printf()` 的字符串实现彩色日志，无需手写转义序列。
- 若需在 `project/Drivers/Gsv650x/userapp/stm32f030c8_gsv6505_gsv_demo/apps/av_main.c` 中显式绕过 GSV 抽象层访问 GSV6715 mailbox，可直接声明 `extern I2C_HandleTypeDef hi2c1;`，再用 `HAL_I2C_Mem_Read(&hi2c1, 0xB0, 0x07, I2C_MEMADD_SIZE_8BIT, ...)` 读取 `HDMI Input Cable Status`。
- `av_main.c` 的主循环很快；若直接在循环中每次都读 mailbox 并打印 RTT，很容易把 `I2C1` 和 RTT 输出刷爆。更稳妥的调试方式是加固定轮询周期，并且只在首次成功或读值变化时打印。
<!-- CODEX_EDITABLE_END -->
