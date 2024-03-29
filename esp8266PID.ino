/* 调试类
*功能：通过串口打印出用户自定义的调试编号
*输入：数据类型 int
*输出：串口字符
*返回：void
*接口：class.dbgInfo(uint)
*/
class Debug{

    private:
         //unsigned int debugInfo;
    public:
        Debug(){   
        }
        void dbgInfo(unsigned int userInfo,char inChar){          
            Serial.print("___________Debug number: ");   
            Serial.println(userInfo);
            Serial.print("debug char = ");
            Serial.println(inChar);
        }
};

/*LED控制
*功能：初始化MCU引脚 控制引脚
*输入:引脚编号 int 
*接口：class(pinDefine) class.pinHIGH() class.pinLOW()
*/
class Pin{
    private:
        int ledPin;
    public:
        Pin(int userPinDefined){ // class Led 构造函数
            ledPin = userPinDefined;
            pinMode(ledPin,OUTPUT); //pin initializ
        }

        void pinHIGH(void){
            digitalWrite(ledPin,1);
        }
        void pinLOW(void){
            digitalWrite(ledPin,0);
        }
};

/*沿边检测类
*功能:沿边检测 （上升沿+下降沿）
*输入：数据类型 bool
*返回值：数据类型 bool'
*接口：class.r_edge(bool);class.f_edge(bool)
*/
class Edge{

    private:
        bool inputBit;
        bool r_temp;   //rising temp bit
        bool r_edgeOut;   //Rising Edge 
        bool f_temp;   //falling temp bit
        bool f_edgeOut;   //falling edge

    public:
        bool r_edge(bool inputBit){
            r_edgeOut = inputBit && (! r_temp);
            r_temp = inputBit;
            return r_edgeOut;
        }
        bool f_edge(bool inputBit){
            f_edgeOut = (! inputBit) && f_temp;
            f_temp = inputBit;
            return f_edgeOut;
        }
};

/*定时器类
*功能:定时
*输入：数据类型 EN = bool ，PV = unsigned long
*返回值：数据类型 bool'
*接口：class.timer(bool EN,unsigned long PV)  public var
*/
class Timer{
    private:
        unsigned long timeKeeper; //临时存储时间戳
        bool          edge_tmp;   //辅助时间戳上升沿

    public:
        unsigned long ET;//记录定时器已消耗的时间  elapsed time
        unsigned long PV;//接受 preset value
        bool EN;
        bool TT;
        bool DN;

        void timer(bool inputEN,unsigned long inputPV){
            EN = inputEN;
            PV = inputPV; 
            if(EN){
                if(! edge_tmp){ //此处等效if(EN && ! edge_tmp)
                    timeKeeper = millis();//获取MCU系统时间
                }
                edge_tmp = EN;
                
                if(! DN) ET = millis() - timeKeeper; //计算已消耗的时间 前面的IF条件防止ET溢出

                DN = (ET >= PV); //判断计时目标 成立则计时完成位  
                TT = ! DN && EN; //计时中但未满足DN的状态 
            }
            else{
                timeKeeper = 0;
                edge_tmp   = 0;
                ET = 0;
                DN = 0;
                TT = 0;
            }
        }
};

/*class PID
*功能:PID运算
*输入：数据类型float ;
*返回值：数据类型 float'
*接口：public var ;class(float setVal, float feedback, float setKp, float setKi, float steKd)
*/
class Pid{
    private:

    public:
        float setValue, feedbackVal;
        float kp, ki, kd; //Proportional Integral Derivative
        float pidOut; // result //暴露该接口方便在外部逻辑中限幅
        float ek, ek1, ek2; //存储误差值(当前，上次，上上次)
        float locSum;       //累计积分
        float entropy;      //存储熵增值


        Pid(){  //PID 构造函数
            locSum =0.0; pidOut =0.0;
            kp = 0.0; ki = 0.0; kd = 0.0;
        }

        float pidCalc(float setVal,float feedback,float setKp,float setKi,float steKd){
            setValue    = setVal;
            feedbackVal = feedback;
            kp = setKp; 
            ki = setKi; 
            kd = steKd;
            /*计算当前误差*/
            ek = setValue - feedbackVal;

            /*积分控制逻辑*/
            if(feedbackVal > (setValue * 0.20)){//限制积分运算工作范围 反馈值大于设定值的30%才启用 避免积分过度导致的扰动
                if(setValue != feedbackVal) {   //如果调整目标的反馈值不达预期
                    if(abs(ek - (ek + ek1 +ek2) / 3) < 0.6) //且近3次历史数据的波动值小于0.6 则开始积分 //
                         locSum += ek;
                }       
            }
            /*积分变量限幅*/
            if(locSum > setValue ){
                locSum = setValue;
            } 
            else if(locSum < -setValue) {
                locSum = -setValue;
            }

            /*PID运算+控制输出*/
            pidOut = (kp * ek) + (ki * locSum) + (kd * (ek - ek1));

            /*PID输出限幅处理*/
            if(pidOut > 1000.0){
                pidOut = 1000.0;
            }/*限幅的值按照使用场景自定义*/
            else if(pidOut < 0.0){
                pidOut = 0.0;
            }

            /*历史误差数据存储*/
            ek2 = ek1;
            ek1 = ek;

            return pidOut;
        }
};
/*
*函数功能：输出帮助菜单
*使用方法：直接调用 helpMsg();
*/
void helpMsg(void)
	{
		Serial.println(" Arduino PID sim v2.0  By Qinjunfeng 20211103");
        Serial.println("_______________________________________________");
        Serial.println("'w' --->     print PWM -_-_-_-_ ");
        Serial.println("'qQ'--->     quit;");
        Serial.println("'x' --->     stop ! ");
        Serial.println("'R' --->     Reset ! ");
        Serial.println("'r' --->     running ... ... ...");
        Serial.println("'f' --->     send Pid feedback;");
		Serial.println("'h' --->     help information  (this);");
		Serial.println("'v' --->     send PID controller info;");	
		Serial.println("'s' --->     change PID setValue;   use '+' '-'");
        Serial.println("'e' --->     change entropy;        use '+' '-'");
		Serial.println("'k' --->     change cycle interval; use '+' '-'");
		Serial.println("'p' --->     change Kp ;            use '+' '-'");
		Serial.println("'i' --->     change Ki ;            use '+' '-'");
		Serial.println("'d' --->     change Kd ;            use '+' '-'");
        Serial.println("________________________________________________");
        Serial.println("\n");
	};


/*
*功能:*继承多个类*
*输入:
*返回值: 
*接口: 
*/
class Building : public Timer , public Pid{
    private:

    public:
        int mul_int = 100;//it test

        void sendTMRmsg(){ //该成员函数用于批量打印timer数据
            Serial.println("---show Timer info---");
            Serial.print("Tmr .EN = ");Serial.println(EN);
            Serial.print("Tmr .PV = ");Serial.println(PV);
            Serial.print("Tmr .ET = ");Serial.println(ET);
            //Serial.print("Tmr .TT = ");Serial.println(TT);
            Serial.print("Tmr .DN = ");Serial.println(DN);
            Serial.print("\n");
        }
        void sendPIDmsg(void){ //该成员函数用于批量打印PID数据
            Serial.println("---show PID info---");
            Serial.print("setValue    = ");Serial.println(setValue);
            Serial.print("feedbackVal = ");Serial.println(feedbackVal);
            Serial.print("pidOut      = ");Serial.println(pidOut);
            Serial.print("locSum      = ");Serial.println(locSum);
            Serial.print("kp          = ");Serial.println(kp);
            Serial.print("ki          = ");Serial.println(ki);
            Serial.print("kd          = ");Serial.println(kd);
            Serial.print("ek          = ");Serial.println(ek);
            Serial.print("ek1         = ");Serial.println(ek1);
            Serial.print("ek2         = ");Serial.println(ek2);
            Serial.print("entropy     = ");Serial.println(entropy);
            Serial.print("\n");
        }        
};


/*全局变量*/

//ESP8266 RGB-LED
#define R_led 4
#define G_led 2
#define B_led 5

//h=帮助、v=打印参数、f=返回值、r=PID运行、x=停止、s=设定目标、e=熵增、k= 
char cmdList[15]={'q','h','v','f','r','x','s','e','k','w','p','i','d','R','\0'};//字符操作命令操作列表

//LED控制对象 //初始化引脚控制LED
Pin ledCtl_R(R_led);  //
Pin ledCtl_G(G_led);
Pin ledCtl_B(B_led);


//Debug对象
//Debug debug;     

//交互信息上升沿对象
Edge edge_h,edge_v,edge_f,edge_x,edge_r,edge_s,edge_e,edge_k,edge_w,edge_p,edge_i,edge_d,edge_R;   
//
Edge pulse_x_Hz;

//timer&Pid继承类对象
Building Multi;  

//n_Hz脉冲定时器 pv值 = （pid定时器pv ÷ 10）
Timer TMR_x_Hz;

bool pidRun; //PID开关 true/false = running/stop 
bool pulse;  //用于PID调整时的脉冲
bool cmdMatch; //当来自串口的命令与 switch中的字符匹配状态 = true 否则 = false 


char cmdOption;//存储用户操作项的命令
char cmdAdj ;//存储用户调整参数的命令

int waveform; //PWM波形显示变量

float ControlValue; //PID控制器运算完毕之后输出的控制值 用来控制物理量的输出强度



//PowerUP
void setup() {
    Serial.begin(19200); //初始化Arduino串口参数   
    Serial.setTimeout(5);//设置串行数据等待时间
    
    //参数初始化
    do{
        //pidRun = 1;             //默认上电之后PID开始运行
        //cmdOption = 'f';          //上电之后默认输出 Pid feedback value
        //cmdMatch = 1;           //上电之后让 cmdOption 保持持有字符 ‘f’

        Multi.PV = 1000;        //PID 运算间隔 1000ms

        Multi.setValue = 360.0; //PID 调整目标 360
        Multi.kp = 8.0;         //PID KP默认参数
        Multi.ki = 0.3;         //PID ki默认参数
        Multi.kd = 0.5;         //PID kd默认参数
        Multi.entropy  = -0.58;        //模拟PID熵增程度 消耗的能量

    }while(0);// do While（0） 在此处的意义仅为了聚合操作类型相同的语句

    helpMsg();//上电之后输出一次帮助信息 + 当前参数；
    Multi.sendTMRmsg();
    Multi.sendPIDmsg();   
}



//LOOP    
void loop() {

   
    /*交互命令处理逻辑*/
    if(Serial.available() > 0){//当来自串口的字节数大于0表示有用户命令通过串口传入
        if(! cmdMatch){
            cmdOption = Serial.read();//获取串口的字符串命令
            //delay(5);
        }
        else{
            //一旦 cmdOption 和预设命令匹配成功 来自串口的命令字符将被分流到cmdAdj，在此期间cmdItem不接收用户命令
            //若要 cmdOption 再一次接收命令时只需要将 字符‘q’输入，当系统检测到字符'q'则会将 cmdMatch 置为 false 此时 if语句成立。
            cmdAdj = Serial.read();//获取串口的字符串命令
            //delay(5);
        }
    }

    //交互信息处理
    //{'q','h','v','f','r','x','s','e','k','w','p','i','d','R',\0'}
    do{ 
        if(edge_R.r_edge('R' == cmdOption)){ //如果接收到‘R’字符重置系统但不停止
            cmdOption = '#';
            Multi.feedbackVal = 0.0;
            Multi.locSum      = 0.0;
            cmdOption = 'f'; //当用户输入重置命令时：①PID仿真器运行 ②MCU会通过串口输出feedback值
            Serial.println("............Reset............");
        }

        if(edge_h.r_edge('h' == cmdOption)){ //如果接收到‘h’字符则输出help信息
            cmdOption = '#';
            helpMsg();
        }

        if(edge_v.r_edge('v' == cmdOption)){ //如果接收到‘v’字符则输出TMR & PID参数信息
            Multi.sendTMRmsg();
            Multi.sendPIDmsg();
            cmdOption = '#';
        }

        if(edge_r.r_edge('r' == cmdOption)){ //如果接收到‘r’字符则 运行PID仿真器
            pidRun = 1;
            Serial.println("---  PID running  ---");
            cmdOption = 'f'; //当用户输入运行命令时：①PID仿真器运行 ②MCU会通过串口输出feedback值
        }

        if(edge_x.r_edge('x' == cmdOption)){ //如果接收到‘x’字符则 暂停PID仿真器 再次'r'后可在中断的地方继续运行
            pidRun = 0;
            Serial.println("---  STOP  ---");
            cmdOption = '#';
        }

        if(edge_s.r_edge('s' == cmdOption)){ //如果接收到‘s’字符则 进入后面修改setValue的位置
            Serial.println("change PID setValue .");
            //cmdOption = chClear;  需要进入第二级调整的注释该语句
            cmdMatch = 1;//串口命令字符与预设字符匹配成功 cmdOption 将持有固定的命令字符
        }

        if(edge_e.r_edge('e' == cmdOption)){ //如果接收到‘e’字符则 进入后面Switch 修改熵增幅度
            Serial.println("change PID entropy .");
            //cmdOption = chClear;  需要进入第二级调整的注释该语句
            cmdMatch = 1;//串口命令字符与预设字符匹配成功 cmdOption 将持有固定的命令字符
        }

        if(edge_k.r_edge('k' == cmdOption)){ //如果接收到‘k’字符则 进入后面Switch 修改PID运算间隔时间
            Serial.println("change Cycle interval.");
            //cmdOption = chClear;  需要进入Switch 的项目注释该语句
            cmdMatch = 1;//串口命令字符与预设字符匹配成功 cmdOption 将持有固定的命令字符
        }

        if(edge_p.r_edge('p' == cmdOption)){//如果接收到‘p’字符则 进入后面Switch 修改PID.kp
            Serial.println("Set ___Kp .");
            //cmdOption = chClear;  需要进入Switch 的项目注释该语句
            cmdMatch = 1;//串口命令字符与预设字符匹配成功 cmdOption 将持有固定的命令字符
        }

        if(edge_i.r_edge('i' == cmdOption)){//如果接收到‘i’字符则 进入后面Switch 修改PID.ki
            Serial.println("Set ___Ki .");
            //cmdOption = chClear;  需要进入Switch 的项目注释该语句
            cmdMatch = 1;//串口命令字符与预设字符匹配成功 cmdOption 将持有固定的命令字符
        }

        if(edge_d.r_edge('d' == cmdOption)){//如果接收到‘d’字符则 进入后面Switch 修改PID.kd
            Serial.println("Set ___Kd .");
            //cmdOption = chClear;  需要进入Switch 的项目注释该语句
            cmdMatch = 1;//串口命令字符与预设字符匹配成功 cmdOption 将持有固定的命令字符
        }

        if('f' == cmdOption){ //如果接收到‘f’字符则输出PID feedback value信息
            if(edge_f.r_edge('f' == cmdOption)){ //上升沿保证其只赋值一次
                cmdMatch = 1;//串口命令字符与预设字符匹配成功 cmdOption 将持有固定的命令字符
                Serial.println("print feedback Value.");
            }
            if(Multi.DN){ //feedback 的输出频次由定时器.pv决定
                Serial.println(Multi.feedbackVal);
            } 
            //cmdOption = chClear;  需保持输出的项目注释该语句
        }
        edge_f.r_edge('f' == cmdOption);

        if(pidRun && 'w' == cmdOption){//若接收到'w'字符则输出PID的PWM波形
            if(edge_w.r_edge('w' == cmdOption)){
                Serial.println("show PWM .");
                cmdMatch = 1;//串口命令字符与预设字符匹配成功 cmdOption 将持有固定的命令字符
            } 
            if(TMR_x_Hz.DN){ //为了不让PWM刷新太快
                Serial.println(waveform); //waveform PWM 波形
            }
            
            //cmdOption = chClear;  需保持输出的项目注释该语句
        }
        edge_w.r_edge('w' == cmdOption);
    }while(0); // do While（0） 在此处的意义仅为了聚合操作类型相同的语句

    /*参数调整逻辑*/
    //需要进入Switch的命令有：s,e,k,p,i,d 在调整参数之后需要按下 q 退出修改模式
    switch(cmdOption){
        case 's':

            switch(cmdAdj){
                case '+'://
                    Multi.setValue += 36.0 ;
                    Serial.print("set PID setValue = "); 
                    Serial.println(Multi.setValue);  
                break;

                case '-'://
                    Multi.setValue -= 36.0;
                    Serial.print("set PID setValue = "); 
                    Serial.println(Multi.setValue);
                break;

                case '#':   
                case 'Q':   
                case 'q':   
                case '\0':    break;

                default:
                    Serial.println("Command error !");//错误的命令将输出提示
            }
        break;
            
        case 'e':

            switch(cmdAdj){
                case '+'://
                    Multi.entropy += 0.111 ;
                    Serial.print("set Entropy = "); 
                    Serial.println(Multi.entropy);
                break;

                case '-'://
                    Multi.entropy -= 0.111 ;
                    Serial.print("set Entropy = "); 
                    Serial.println(Multi.entropy);
                break;

                case '#':   
                case 'Q':   
                case 'q':   
                case '\0':    break;

                default:
                    Serial.println("Command error !");//错误的命令将输出提示
            }        
        break;

        case 'k':

            switch(cmdAdj){
                case '+'://
                    Multi.PV += 100 ;
                    Serial.print("set Cycle interval = "); 
                    Serial.println(Multi.PV);
                break;

                case '-'://
                    Multi.PV -= 100 ;
                    Serial.print("set Cycle interval = "); 
                    Serial.println(Multi.PV);
                break;

                case '#':   
                case 'Q':   
                case 'q':   
                case '\0':    break;

                default:
                    Serial.println("Command error !");//错误的命令将输出提示
            }        
        break;

        case 'p':

            switch(cmdAdj){
                case '+'://
                    Multi.kp += 0.1 ;
                    Serial.print("set Kp = "); 
                    Serial.println(Multi.kp);
                break;

                case '-'://
                    Multi.kp -= 0.1 ;
                    Serial.print("set Kp = "); 
                    Serial.println(Multi.kp);
                break;  

                case '#':   
                case 'Q':   
                case 'q':   
                case '\0':    break;

                default:
                    Serial.println("Command error !");//错误的命令将输出提示      
            }
        break;

        case 'i':

            switch(cmdAdj){
                case '+'://
                    Multi.ki += 0.02 ;
                    Serial.print("set Ki = "); 
                    Serial.println(Multi.ki);
                break;

                case '-'://
                    Multi.ki -= 0.02 ;
                    Serial.print("set Ki = "); 
                    Serial.println(Multi.ki);
                break;  

                case '#':   
                case 'Q':   
                case 'q':   
                case '\0':    break;

                default:
                    Serial.println("Command error !");//错误的命令将输出提示      
            }
        break;

        case 'd':

            switch(cmdAdj){
                case '+'://
                    Multi.kd += 0.02 ;
                    Serial.print("set Kd = "); 
                    Serial.println(Multi.kd);
                break;

                case '-'://
                    Multi.kd -= 0.02 ;
                    Serial.print("set Kd = "); 
                    Serial.println(Multi.kd);
                break;   

                case '#':   
                case 'Q':   
                case 'q':   
                case '\0':    break;

                default:
                    Serial.println("Command error !");//错误的命令将输出提示     
            }
        break;

        case 'R':   
        case 'h':   
        case 'v':   
        case 'r':   
        case 'x':   
        case 'f':   
        case 'w':   
        case '#':   
        case 'q':   
        case 'Q':   
        case '\0':  break;//power UP之后cmdOption中是空字符也就是 '\0' 你无法显式的识别它

        default:
            Serial.println("Command error !!!");//错误的命令将输出提示
            cmdOption = '#';//这样的处理使 "Command error !!!" 只给用户输出一次
    }

    //当参数调整结束键入命令'q' 才可以执行其他的操作
    if('q' == cmdAdj || 'Q' == cmdAdj){
        cmdMatch = 0; //如果if条件满足 cmdOption 就能接收用户新的命令
        cmdOption = '#';
        cmdAdj  = '#';
        Serial.println("quit !");
    }
    cmdAdj = '#'; //为防止同一个调整命令被多次执行，每一个程序运行的循环将cmdAdj中的调整命令清除


    Multi.timer(pidRun && (! Multi.DN), Multi.PV); //PIDctr Timer  

    if(Multi.DN){   
        //PID控制器调用
        ControlValue = Multi.pidCalc(Multi.setValue,Multi.feedbackVal,Multi.kp,Multi.ki,Multi.kd);
        
        Multi.feedbackVal += Multi.entropy; //模拟物理的熵增  

    }

    //TMR_x_Hz.timer的PV = Multi.PV的十分之一
    TMR_x_Hz.timer(pidRun && (! TMR_x_Hz.DN) , Multi.PV / 10); 

    //获得默认10Hz的脉冲 10Hz是相对于PID间隔时间；
    pulse = pulse_x_Hz.f_edge(TMR_x_Hz.DN);

    // 1、将ControlVal转换成 PWM波形  2、LED和PWM 波形同步闪烁
    if(pidRun && Multi.ET <= ControlValue){
        waveform = 1;
        ledCtl_R.pinHIGH();//LED ON  
    }
    else{
        waveform = 0;
        ledCtl_R.pinLOW();// LED OFF
    }

    //PWM的高电平持续时间越长 Feedback的值增加的越多，调整力度越激进，反之调整力度越柔和；
    if(pidRun && pulse && waveform){
        Multi.feedbackVal += 0.5;
    }

    //如果PID控制器将反馈值调整到设定值允许的范围则点亮绿灯
    if(abs(Multi.setValue - Multi.feedbackVal) < 1.0)
        ledCtl_G.pinHIGH();
    else
        ledCtl_G.pinLOW();

}


