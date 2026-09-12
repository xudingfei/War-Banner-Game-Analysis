# 陷阵之志：随机地图机制


- 安装目录有 376 个 .map 文件，声明尺寸全部为 8×8。这包括特殊地图，不能等同于 376 张普通对战地图。
- 地图记录固定坐标及 terrain、建筑属性，并带 tags 与 zones。例：maps/acid0.map 包含 generic、acid、acid_pool、pistons 标签及 pistons 候选区域。
- scripts/missions/missions.lua 的 GetMapTag() 对标签表调用 random_element；GetMap() 从 MapList 随机取项。GetMapTag 的存在不代表已经还原引擎内全部地图选择逻辑。
- Mission:AddDefended 在地图指定区域里用 random_removal 取位置，随后设置地形及创建单位；mission_tanks.lua 声明 satellite 标签并调用 AddDefended。
- 因此可确认预制布局、任务标签、指定位置随机布置的机制。地图旋转/镜像、最终抽图权重、随机数发生器与调用顺序未完整验证。

开发者 Justin Ma 在采访中明确说明：基本布局人工设计，森林、沙地及任务特有建筑位置采用程序变化，以降低令人无解的布局出现概率。
来源：https://www.gamedeveloper.com/design/devs-weigh-in-on-the-best-ways-to-use-but-not-abuse-procedural-generation
