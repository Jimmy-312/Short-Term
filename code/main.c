#include<reg52.h>
#include<intrins.h>
#include<codetab.h>
#include<LQ12864.h>
#define uint unsigned int
#define uchar unsigned char

#define Nack 10    									//Number of retransmissions in temperature measurement communication
#define ACK(); SDA=0;NOP_3();SCL=1;NOP_3();SCL=0;	//ACK signal

/*Address of the temperature threshold in the EEPROM*/
#define Thread1 0x01   
#define Thread2 0x02   
#define Thread3 0x03

/*Address definition in EEPROM*/
#define Hisn 0x00	  				//History pointer
#define EEPDa 0x04					//data head
#define Naddr 0x30					//current name data
#define Nowaddr 0xE0				//history name data

/*pin definition*/   
sbit RS=P1^2;             
sbit RW=P1^1;          
sbit LCDE=P1^0;
sbit LED=P1^6;
sbit BUZZ=P1^7;
sbit INT2=P3^5; 
sbit DC=P1^5;         

/*bit flag definition*/
bdata uchar flag;
sbit bit_out=flag^1;
sbit bit_in=flag^0;
sbit sta=flag^2;	        //Temperature measurement flag bit       
sbit set=flag^3;			//Threshold setting flag bit
sbit his=flag^4;			//History flag bit
sbit sw=flag^5;				//History display refresh flag bit
sbit setw=flag^6;			//Threshold up bit
sbit setd=flag^7;			//Threshold display refresh flag bit

bdata uchar flag2;
sbit setm=flag2^0;		    //Threshold down bit
sbit bluea=flag2^1;			//Bluetooth active bit
sbit xname=flag2^2;			//Edit name bit
sbit blues=flag2^3;			//Bluetooth send bit
sbit psend=flag2^4;         //Bluetooth send method bit

/*EEPROM data and flag*/
uchar bdata EEP;
sbit EEP_7 = EEP^7;
sbit EEP_0 = EEP^0; 

/*LCD function definition*/
void busy(void);						//check if busy
void cmd_w(uchar cmd);					//write the command
void init1602(void);					//initial the LCD
void dat_wrt(uchar dat); 				//wtite the data
void LCD_print(uchar *str,uchar n);		//display the str

/*EEPROM function definition*/
void I2C_Ack(void);						//wait the reply
void I2C_Start(void);					//start signal
void I2C_Stop(void);					//stop signal
void Write_Byte(uchar wdata);			//write a byte
uchar Read_Byte(void);					//read a byte
uchar eread(uchar addr);				//read a byte data 
void ewrite(uchar addr,uchar dat);		//write a byte data
void ewrstr(uchar addr,uchar* str);		//wrtie a str
void erdstr(uchar addr);				//read a str

/*MLX90614 function definition*/
void i2c_Init(void);					//initial  IIC
void start_bit(void);					//start signal
void stop_bit(void);					//stop signal
void send(uchar dat_byte);				//write a byte
void send_bit(void);					//write a bit
uchar read(void);						//read a byte
void receive_bit(void);					//read a bit
uint gettemp(void);	 					//get the target temp
void getETemp(void);					//get the evironment temp
void mtemp(void);						//measure module

/*Bluetooth function definition*/
void ConfigUART(uint baud);				//initial
void BlueSend(uchar *str);				//send a str
void BlueReceive();						//receive a str

/*custom function definition*/
void inital(); 						//initial everything
void time1int();				   	//timer 1 initial
void dismain();						//display the mainpage
void hisswitch(void);				//history switching
void SendHis();						//Send all history record
void sta_mode();					//Measure mode
void set_mode();				   	//threadhold set mode
void his_mode();					//history record view mode
void edit_name();					//edit user name 

/*Variable Definition*/
uint count=0;				  //counter
uchar flag1;				  //transport use byte
uchar evt[7];				  //store the environment temp
uchar RxdB;					  //store the sbuf
uint tem;					  //store the temp
float otem;					  //store the original temp
uint temthread;				  //store the temp treadhold
uchar hisnum;				  //store the history index
uchar s[10];				  //store the str mostly
uchar sn[9];				  //store the name str
uchar sel;					  //store the history record select index
uchar td[3];				  //store the threadhold in array
uchar index[2];				  //store the history record index str
uchar temH,temL,err;		  //store the original temp (vvvvvvvery original) and some useless things

void main()
{
	inital();
	
	while(1)	   //main loop
	{    
		if(sta)
		{
			sta_mode();				//Various modes are included here for switching
		}
		else if(set)
		{
			set_mode();
		}
		else if(his)
		{	
			his_mode();
		}
		else
		{
		 	if(xname){ 
				edit_name();
			}
		}
	}
}

////////////////Function module//////////////
void sta_mode()
{
	LCD_print("  Result:",1);
	OLED_CLS();
	OLED_P8x16Str(0,0,"Result:");
	mtemp();
	getETemp();							   //Temperature measurement and display
 	sta=0;
	LCD_print(s,0);
	OLED_P8x16Str(56,0,s);
	LCD_print("       Evn: ",0);
	OLED_P8x16Str(0,2,"Evn:");
	OLED_P8x16Str(32,2,evt);
	LCD_print(evt,0);
	if(tem>temthread){			//Alert module
		BUZZ=0;
		LED=0;
		delay(1000);
		BUZZ=1;
		LED=1;

		OLED_P16(0,4,7);
		OLED_P16(16,4,8);
	}else{
		OLED_P16(0,4,19);
		OLED_P16(16,4,20);
	}  
	
	hisnum++;
	if(hisnum==9)
		hisnum=1;  				  //loop store
	
	ewrite(Hisn,hisnum);
    ewrite(EEPDa+hisnum*3,tem/1000);  
    ewrite(EEPDa+hisnum*3+1,tem % 1000/100);  		 //Storing data to EEPROM
 	ewrite(EEPDa+hisnum*3+2,tem%100/10); 

	ewrstr(Naddr+hisnum*8,sn);
	delay(2000);
	dismain();
}

void set_mode(){
	uchar i;
	if(!setd)						   //display module
	{
		s[0]=td[0]+0x30;
		s[1]=td[1]+0x30;
		s[3]=td[2]+0x30;
		s[2]='.';
		s[4]='C';
		s[5]=0;
		LCD_print("   Set:",1);
		LCD_print(s,0);
		OLED_CLS();
		OLED_P8x16Str(0,0,"Set:");
		OLED_P8x16Str(32,0,s);
		setd=1;
		OLED_P16(0,4,9);
		for(i=0;i<4;i++)
			OLED_P16(i*16+16,4,9+i);
	}
	if(setw)					//up the treadhold
	{
		td[2]+=1;
		if(td[2]==10)
		{
			td[1]+=1;
			td[2]=0;
		}
		ewrite(Thread3,td[2]);
		ewrite(Thread2,td[1]);
		temthread=td[0]*1000+td[1]*100+td[2]*10;
		setd=0;
		setw=0;
	}
	if(setm)			   			//down the treadhold
	{
		if(td[2]==0)
		{
			td[1]-=1;
			td[2]=10;
		}
		td[2]-=1;
		ewrite(Thread3,td[2]);
		ewrite(Thread2,td[1]);
		temthread=td[0]*1000+td[1]*100+td[2]*10;
		setd=0;
		setm=0;
	}
}

void his_mode()
{
	if(sw)
	{
		if(sel==0)
			sel=8;								 //display module
		s[0]=eread(EEPDa+sel*3)+0x30;
		s[1]=eread(EEPDa+sel*3+1)+0x30;
		s[3]=eread(EEPDa+sel*3+2)+0x30;
		s[2]='.';
		s[4]='C';
		s[5]='\0';

		if(sel<=hisnum){
			index[0]=hisnum+1-sel+0x30;
		}else{
			index[0]=9+hisnum-sel+0x30;
		}
		LCD_print(" History: ",1);	
		OLED_CLS();
		OLED_P8x16Str(0,0,"History:");
		erdstr(Naddr+sel*8); 
		LCD_print(s,0);
		OLED_P8x16Str(64,0,s);
		LCD_print("   ",0);
		index[1]=0;
		LCD_print(index,0);
		OLED_P8x16Str(0,2,index);
		LCD_print("  ",0);
		OLED_P8x16Str(24,2,sn);
		LCD_print(sn,0);	
		sw=0;	
		sel--;		   
	}
	if(bluea)							   //send module
	{
		OLED_CLS();
		LCD_print("   Bluetooth A",1);
		OLED_P8x16Str(0,0,"Bluetooth A");
		OLED_P16(0,2,13);
		OLED_P16(16,2,14);
		OLED_P16(32,2,15);
		OLED_P16(48,2,16);
		while(bluea&&blues);				//exit judge
		if(!bluea)
			goto bback;
		bluea=0;
		blues=1;
		OLED_P8x16Str(0,4,"Sending...");
		LCD_print("       Sending...",0);
		SendHis();
		LCD_print("   Bluetooth            OK!",1);
		OLED_P8x16Str(0,6,"OK!");							 //send module
		BUZZ=0;
		delay(500);
		BUZZ=1; 
		EX0=1;
		EX1=1;
bback:	LED=1;
		time1int();				
		sel=hisnum;
		sw=1;
	}
}

void edit_name()
{
		BlueReceive();
		if(!xname)
			goto bbback;		 //exit judge
		ewrstr(Nowaddr,s);
		xname=0;  
		erdstr(Nowaddr);
		LCD_print(sn,0);
		OLED_P8x16Str(40,0,sn);
		cmd_w(0x4C+0x80);
		LCD_print("OK!",0);				 //receive module
		OLED_P8x16Str(0,2,"OK!");
		delay(2000);
bbback: time1int();
		dismain();
}

void SendHis()
{	
	uchar i,j;
	EX0=0;
	EX1=0;
	j=hisnum;
	for(i=0;i<8;i++,j--)			  //loop send the 8 record
	{
		if(j==0)
			j=8;
		s[0]=eread(EEPDa+j*3)+0x30;
		s[1]=eread(EEPDa+j*3+1)+0x30;
		s[3]=eread(EEPDa+j*3+2)+0x30;
		s[2]='.';
		s[4]='C';
		s[5]=0x20;
		s[6]=i+1+0x30;
		s[7]=0x20;
		s[8]=0;
		BlueSend(s);
		erdstr(Naddr+j*8);
		BlueSend(sn);
		s[0]='\r';					//line break
		s[1]='\n';
		s[2]=0;
		BlueSend(s);
	}
	s[0]='O';
	s[1]='K';
	s[2]='!';
	s[3]='\0';
	BlueSend(s);
}

//////////////initalized module//////////////////
void dismain()
{
	uchar i;
	OLED_CLS();
	erdstr(Nowaddr);
	LCD_print(sn,1);
	OLED_P8x16Str(0,0,sn);
	cmd_w(0x88);									 //welcome display and some initial
	LCD_print("Welcome!  Press to START",0);
	OLED_P8x16Str(0,2,"Welcome!");
	OLED_P8x16Str(0,4,"Press to START");
	for(i=0; i<7; i++)
	{
	 	OLED_P16(i*16,6,i);						 //loop for Chinese characters
	}
}

void time1int()
{
	ES=0;
	TMOD &= 0x0F;
	TMOD |= 0x60;				 //timer 1 initial, use for interrupting
	TH1=0xFF;
	TL1=0xFF;
	ET1=1;
	TR1=1;
}

void inital()
{
	EX0 = 1;        
    IT0 = 1;                
    EX1 = 1;        
    IT1 = 1;
	EA=1; 
	sta=0;
	his=0;
	sw=0;						 //some default value
	setw=0;
	setd=0;
	xname=0;
	bluea=0;
	setm=0;
	sta=0;
	set=0;
	DC=0;
	blues=1;
	psend=0;

	time1int();					//time1 and screen initial
	init1602();
	OLED_Init();
	//ewrite(Hisn,0);	 
	td[0]=eread(Thread1);
	td[1]=eread(Thread2);					  //get the temp threadhold
	td[2]=eread(Thread3);
	temthread=td[0]*1000+td[1]*100+td[2]*10;
	hisnum=eread(Hisn);	
	dismain();
}

////////////////////interrupt module////////////////////
void int0() interrupt 0
{
	delay(100);
	if(INT0==0)
	{
		if(his)
		{
			//LED=1;
			if(bluea){
				if(INT2==0){						  //transport mode switcher
					psend=~psend;
					OLED_CLS();
					if(!psend){
						LCD_print("   Bluetooth A",1);
						OLED_P8x16Str(0,0,"Bluetooth A");
						OLED_P16(0,2,13);
						OLED_P16(16,2,14);
						OLED_P16(32,2,15);
						OLED_P16(48,2,16);
					}else{
						LCD_print("   Bluetooth B",1);
						OLED_P8x16Str(0,0,"Bluetooth B");
						OLED_P16(0,2,17);
						OLED_P16(16,2,18);
						OLED_P16(32,2,15);
						OLED_P16(48,2,16);
					}
				}else{
			   		bluea=0;			//exit send
				}
			}else{
				set=1;
				setd=0;				   //switch to set
				his=0;
			}
		}else
		if(set){
			set=0;
			his=0;					  //switch to homepage
			dismain();
		}else{
			if(xname){
				xname=0;			  //exit edit name
			}else{
			 	his=1;
				//LED=0;
				sel=hisnum;			   //switch to history mode
				sw=1;
			}
		}
	}
}

void int1() interrupt 2
{
	uchar i;
	delay(100);
	if(INT1==0)
	{
		if(his)
		{
			if(bluea){
			  	blues=0;	 		//start send
			}else{
				sw=1;				//switch record
			}			
		}else
		if(set)
		{
			if(INT0==0){
				ewrite(Thread1,3);
				ewrite(Thread2,7);			  //reset temp threadhold
				ewrite(Thread3,6);
				td[0]=eread(Thread1);
				td[1]=eread(Thread2);
				td[2]=eread(Thread3);
				setw=1;
				setd=0;
				return;
			}
			if(INT2==0){
				LCD_print("Create By Jimmy     2021.7.25",1);
				OLED_CLS();
				OLED_P8x16Str(0,0,"Create By Jimmy");			//display the author info
				OLED_P8x16Str(0,2,"2021.7.26");
				for(i=0; i<8; i++)
				{
				 	OLED_P16x16(i*16,6,i);
				}
				delay(5000);
				inital();
				return;
			}
			setw=1;					  //up threadhold
		}else{
			if(xname){
				xname=0;		 //exit edit name
			}else{
				sta=1;			 //switch to homepage
			}
		}
	}
} 

void int2() interrupt 3
{
	delay(100);
	if(INT2==0)
	{
		if(his)
		{
			if(bluea){

			}else{
				bluea=1;			//enter in the record send --- Bluetooth default mode A
				LED=0;
				ConfigUART(9600);
			}				
		}else
		if(set)
		{
			setm=1;					 //down threadhold
		}else{
			xname=1;
			OLED_CLS();
			LCD_print("Name: ",1);			  //enter the name edit
			OLED_P8x16Str(0,0,"Name:");
			ConfigUART(9600);
		}
	}
} 

/////////////////Temperature measurement module////////////////
void mtemp(void)
{
	uchar i;
	tem=0;
	i2c_Init();
	for(i=0;i<10;i++)
		tem=tem+gettemp();			//measure 10 times and get the average value
		delay(10);
	otem=tem/1000;
	tem=100*(otem*otem*otem*(-0.0012)+0.1719*otem*otem+125.7-7.06719*otem);			   //temp conversion formula
	s[0]=tem/1000+0x30;
	s[1]=tem % 1000/100+0x30;
	s[2]=0x2E;
	s[3]=tem%100/10+0x30;				 //int2str
	s[4]=0x43;
	s[5]='\0';
}

uint getTemp(void)
{
	start_bit();
	send(0xB4); 			   //IIC address
	send(0x07);  			   //Ram address
	start_bit();
	send(0x01);
	bit_out=0;				   //read
	temL=read();
	bit_out=0;
	temH=read();
	bit_out=1;
	err=read();
	stop_bit();
	return (temH*256+temL)*2-27315;			 //initial conversion formula
}

void getETemp(void)
{
	uint temtem;
	start_bit();
	send(0xB4); 
	send(0x06);  			  //Ram address
	start_bit();
	send(0x01);
	bit_out=0;				  //the same as before
	temL=read();
	bit_out=0;
	temH=read();
	bit_out=1;
	err=read();
	stop_bit();
	temtem=(temH*256+temL)*2-27315;
	evt[0]=temtem/1000+0x30;
	evt[1]=temtem%1000/100+0x30;
	evt[2]='.';								//int2str
	evt[3]=temtem%100/10+0x30;
	evt[4]='C';
	evt[5]=0;
}

///////////////MLX90614 transport module/////////////////
void i2c_Init(void)
{
	SCL=1;
	SDA=1;
	_nop_();
	_nop_();				   //initial signal
	_nop_();
	_nop_();
	SCL=0;
	delay(1);
	SCL=1;
}

void start_bit(void)
{
	SDA=1;
	_nop_();_nop_();_nop_();_nop_();_nop_();
	SCL=1;
	_nop_();_nop_();_nop_();_nop_();_nop_();
	SDA=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();			 //follow the transport protocols
	SCL=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();
}

void stop_bit(void)
{
	SCL=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();
	SDA=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();			 //follow the transport protocols
	SCL=1;
	_nop_();_nop_();_nop_();_nop_();_nop_();
	SDA=1;
}

void send(uchar dat_byte)
{
	char i,n,dat;
	n=Nack;
send:
	dat=dat_byte;
	for(i=0;i<8;i++)
	{
		if(dat&0x80)
			bit_out=1;				  //follow the transport protocols
		else
			bit_out=0;
		send_bit();
		dat=dat<<1;
	}
	receive_bit();
	if(bit_in==1)
	{
		stop_bit();
		if(n!=0)
		{
			n--;
			goto Repeat;			//resend the signal
		}
	}
	goto exit;
Repeat:
	start_bit();
	goto send;
exit: ;
}

void send_bit(void)
{
	if(bit_out==0)
		SDA=0;
	else
		SDA=1;
	_nop_();
	SCL=1;									   //follow the transport protocols
	_nop_();_nop_();_nop_();_nop_();
	_nop_();_nop_();_nop_();_nop_();
	SCL=0;
	_nop_();_nop_();_nop_();_nop_();
	_nop_();_nop_();_nop_();_nop_();
}

uchar read(void)
{
	uchar i,dat;
	dat=0;
	for(i=0;i<8;i++)
	{
		dat=dat<<1;						//follow the transport protocols
		receive_bit();
		if(bit_in==1)
			dat=dat+1;
	}
	send_bit();
	return dat;
}

void receive_bit(void)
{
	SDA=1;bit_in=1;
	SCL=1;
	_nop_();_nop_();_nop_();_nop_();
	_nop_();_nop_();_nop_();_nop_();
	bit_in=SDA;								  //follow the transport protocols
	_nop_();
	SCL=0;
	_nop_();_nop_();_nop_();_nop_();
	_nop_();_nop_();_nop_();_nop_();
}

/////////////Bluetooth module/////////////////
void BlueSend(uchar *str)
{
	uchar i=0;
	while(str[i]!=0){
		TI=0;
Resend: SBUF=str[i];  			   //send a byte
		delay(50);
		if(psend){
			count=0;
			while(!RI){
				count++;
				if(count>50000)				//resend module
					goto Resend;
			}
			RI=0; 
		}
		i++;                                                                   
	}
}

void BlueReceive()
{
	uchar i=0;
	while(xname){
		while(!RI&&xname);
		if(!xname)
			return;					  //receive the new name
		RxdB=SBUF;
		RI=0;
		if(RxdB=='@')
			break;
		s[i++]=RxdB;
	}
	s[i]=0;
}

void ConfigUART(unsigned int baud)
{
	SCON = 0x50; 
	TMOD &= 0x0F; 
	TMOD |= 0x20; 									//initial com bps
	TH1 = 256 - (11059200/12/32)/baud; 
	TL1 = TH1; 
	ET1 = 0; 
	ES = 1; 
	TR1 = 1; 
}

void InterruptUART() interrupt 4
{
	if (RI) 
	{
	//	bluea=0;
	//	SBUF=1;
	//	delay(30);						   //useless
	}
	if (TI) 
	{
	//	TI = 0; 
	}
}

/////////////LCD module/////////////
void cmd_w(uchar cmd)                                  
{
	busy();                                                 
	P2=cmd;                                                 
	RS=0;                      //follow the transport protocols                              
	RW=0;                                                    
	LCDE=1;                                               
	LCDE=0;
}

void init1602(void)                            
{
	cmd_w(0x01);                  //follow the transport protocols               
	cmd_w(0x0c);                               
	cmd_w(0x06);
	cmd_w(0x38);  
}
                               
void busy(void)                                
{
	flag1=0x80;                                    
	while(flag1&0x80)                          
	{
		P2=0xff;					   //follow the transport protocols
		RS=0;                                               
		RW=1;                                             
		LCDE=1;                                       
		flag1=P2;                                       
		LCDE=0;
	}
}

void dat_w(uchar dat)                                  
{
	busy();                                               
	if(flag1==16)
	{	
		P2=0xC0;
	    RS=0;                                               
	    RW=0;                       //follow the transport protocols                                                                 
	    LCDE=1;                                               
	    LCDE=0;
	}
	P2=dat;
	RS=1;                                                       
	RW=0;                                                                                                     
	LCDE=1;                                                   
	LCDE=0;
}

void LCD_print(uchar *str,uchar n)                                      
{
	if(n)
		cmd_w(0x01);
	while(*str!='\0')                                   
	{										  //send str byte by byte
		dat_w(*str);                                  
		str++;                                                
	}
}

////////////EEPROM//////////////
void I2C_Ack(void)		
{
	uchar i;
	SCL=1;
	delay(1);
	while((SDA==1)&&(i<255)) i++;				  //follow the transport protocols
	SCL=0;
	delay(1);
	SDA=1;
}

void I2C_Start(void)
{
	SDA=1;
	delay(1);
	SCL=1;									  //follow the transport protocols
	delay(1);
	SDA=0;
	delay(1);
	SCL=0;
		
}

void I2C_Stop(void)
{
	SDA=0;
	delay(1);
	SCL=1;
	delay(1);							 //follow the transport protocols
	SDA=1;
	delay(1);	
}

void Write_Byte(uchar wdata)   
{
	uchar i;
	EEP=wdata;
	for(i=0;i<8;i++)
	{	
		SDA=EEP_7;
		SCL=1;
		delay(1);
		EEP<<=1;					  //follow the transport protocols
		SCL=0;
		delay(1);
	}
	SDA=1;
	delay(1);
	SCL=0;
}

uchar Read_Byte(void)	 
{
	uchar i;
	SDA=1;
	EEP=0;
	for(i=0;i<8;i++)
	{
		EEP<<=1;				 //follow the transport protocols
		SCL=1;
	    delay(1);
		EEP_0=SDA;
		SCL=0;
		delay(1);
	}
	return EEP;
}

void ewrite(uchar addr,uchar dat)
{
	I2C_Start();
	Write_Byte(0xa0);
	I2C_Ack();
	Write_Byte(addr);  
	I2C_Ack();
	Write_Byte(dat);				   //follow the transport protocols
	I2C_Ack();
	I2C_Stop();
}

uchar eread(uchar addr)
{
	uchar dat;
	I2C_Start();
	Write_Byte(0xa0);
	I2C_Ack();
	Write_Byte(addr);  
	I2C_Ack();
	I2C_Start();						//follow the transport protocols
	Write_Byte(0xa1);
	I2C_Ack();
	dat=Read_Byte();
	I2C_Stop();
	return dat;
}

void erdstr(uchar addr)
{
	uchar i=0;
	do{	 
		sn[i]=eread(addr+i);			  //read str byte by byte
		i++;	
	}while(sn[i-1]!=0);
}

void ewrstr(uchar addr,uchar* str)
{
	uchar i=0;
	do{
		ewrite(addr+i,str[i]);			  //write str byte by byte
		i++;
	}while(str[i-1]!=0);
}