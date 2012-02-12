操作：
打开exe，点击简单合并，然后去喝茶看戏吧= =，新版本速度下降很多，原因见更新记录；
最终输出在
OldIndex.out.txt 旧索引贴的信息
NewIndex.out.txt 合并后的信息
可以用winMerge之类的工具来比较查看，附图Merge1.PNG/Merge2.PNG；


更新记录


v0.007版本
1. 修改了索引输出格式可部分配置；
[Merge]
IndexOutputFormat=%s\n%s\n
2. 修改了部分受大小写问题影响的处理；

P.S.
    大田汝真是该死= =欺负我家硝子……
    哼，如果抹杀存在能影响到三次元世界就好了，咱也不用记得那讨厌的情节了= =

				3:45 2009-8-28


v0.006B版本
应小wo要求整理了配置文件，部分输出文件用了中文名，主程序不变


v0.006版本
C76的大潮中，人参又被FXTZ吃尽了……久违的版本
1. 适应论坛改版
a. 改写了分页读取，目前直接写在配置中了，[TitleUrl] ForcePage 项
b. 改写了Url的读取，修改了正则表达式
c. 新论坛在分页版面的标题信息读取不全，暂时改为从url中打开页面读取标题栏，因此速度有极大减低= =

2. 使用LCMapString API实现了简繁转换，程序中将读入的繁体都转简体；
3. 输出的NewIndex.out.txt带上了颜色信息，不再完全排序，而是只排第一个字母，并在比较简单的层面上程度上实现了同系列统一，待完善；

				6:16 2009-8-24


v0.005版本
1. 用IFELanguage接口实现了汉字反查拼音功能；
2. 实现了一个简单的索引贴合并处理，点击简单合并即可，会生成两个文件；

OldIndex.out.txt
NewIndex.out.txt

是索引贴的前后对比，可以用文件对比工具来查看不同之处。

				8:38 2009-8-7




v0.004版：
1. 看了一下boost的正则表达式解析，用来解析第一页中还有的页数信息，进而下载分页帖子标题，id和所属分版信息并输出；

话说这么一个正则，咱竟然第一次就写对了Orz >_<
^leadbbs\(([0-9]+),([0-9]+),([0-9]+),"(.*?)",([0-9]+),"([0-9]+)",([0-9]+),([0-9]+),"(.*?)",([0-9]+),([0-9]+),"(.*?)",([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),"(.*?)","([0-9]+)",([0-9]+)\);$

				6:42 2009-7-31



v0.003版：
1. 查了一下下载网页源码的方法，索引贴的由手工保存改为自动下载;
2. 加上了运行状态输出功能；

Url如发生变动请配置于config.ini的此处

[OrgIndex]
; 索引贴的Url
OrgIndexUrl=http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=20935

				4:17 2009-7-30



v0.002版：

1. 新增了索引贴的源码解析和修改检查；
2. 修改了部分处理和函数；
3. 修改了配置文件格式；


保存索引贴HTML源码到本地(浏览器查看源代码，另存为即可)
用程序解析，会生成输出结果

[OrgIndex]
;未格式化原索引页Html源码信息粗略提取
;中间文件
TmpFile=OrgIndexTmp.txt
;输出文件
BookInfoFile=OrgIndex_BookInfo.txt
;中间文件校验结果输出
TmpCheckFile=TmpCheck.txt

例如config.ini配置，中间文件生成在OrgIndexTmp.txt，修改意见生成在TmpCheck.txt


				4:26 2009-7-29




=================



这算日志好了：

   配置主要在config.ini，写了部分注释；


1. 新格式：
   新格式因为信息完全定位方便且目前没有输入来测试，所以先不做新格式支持；

2. 原工具的直接输出再处理：

   目前这个是试处理了一下旧格式，输入就是原工具未经人工的输出，看了下输出结果(好多，1/10没看到……) 稍微配置了一下 Replace表和Extra表
   一个是用来做替换的，一个是用来标记注解的
   作者/书名/系列的信息其实也是类似Replace/Extra表一样，作为识别依据，不过量可以想见的庞大……

3. 原索引贴的处理

   因为原索引贴已经有了排序，解析相对乐观，但也存在部分比较特殊的地方，例如：


伯爵与妖精 [谷瑞惠]
一 ～ 五卷 [T3录入制作]
http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=27693
第六卷 被调包的公主 
http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=38046
第七卷 泪水的秘密 
http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=38318
十一、十二、十三卷
http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=48797
第十四卷 圣地是谁的梦想 
http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=48958
十五、十六、十七卷
http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=48982
伯爵与妖精短篇＜两人尚未知晓的奇迹＞
http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=41524
悖德之城The Genetic Sodom ILEGENES(上)[桑原水菜]
http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=40571
悖德之城The Genetic Sodom ILEGENES（下）
http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=40851
把花束献于月亮与你
第一卷:http://www.nxkyz.com/bbs/Announce/Announce.asp?BoardID=331&ID=23586
第二卷:http://www.nxkyz.com/bbs/Announce/Announce.asp?BoardID=331&ID=23749
第三卷:http://www.nxkyz.com/bbs/Announce/Announce.asp?BoardID=331&ID=24054&q=1&r=2463
第四卷:http://www.nxkyz.com/bbs/Announce/Announce.asp?BoardID=331&ID=24480&p=2&q=1&r=2646
第五卷:http://www.nxkyz.com/bbs/Announce/Announce.asp?BoardID=331&ID=25167&p=3&Upflag=1&q=3&r=2637


    这种和通常的一行书名/其他信息，一行链接的方式不同，会造成问题，若只是少量情况，就整理成和其他一样的比较合适



关于整合：

    目前来看，安逸的解决办法是在保留原索引序的情况下做插入排序，不过这样主键会变成用url..更新的时候检查url是否已出现在表内，有则不处理或记录出来(不排除编辑主题贴影响标题之类的情况)，没有就写在合适位置
    虽然新格式的主题互相的顺序可以预期有保证，但旧主题无法不整理同样影响排序，所以需要对旧格式进行最低限度的整理，或慢慢完善；
    重复整理的情况：某个主题是否整理过，由Url决定及当时取下的标题而定……若两次取得的信息一致，则不再处理(前提是之前整理完全了)
    url固定的风险是论坛大变更之类链接全变，不过这样的情况发生，索引的信息本身也没意义了，所以还是可以接受的……

P.S.
   …
   蛋疼的想做unicode工程，不习惯鼓捣了半天，后面发现貌似必要性不大先搁置了，现在只是ANSI编码的……
   嘛，混沌中，说明和预先部分貌似很混乱…趴床……
							9:18 2009-7-28
