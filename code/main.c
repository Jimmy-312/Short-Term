#include<reg52.h>
#include<intrins.h>
#define uint unsigned int
#define uchar unsigned char
#define Nack 10 
#define ACK(); SDA=0;NOP_3();SCL=1;NOP_3();SCL=0;

#define Thread1 0x01
#define Thread2 0x02
#define Thread3 0x03

#define Hisn 0x00
#define EEPDa 0x04
   
sbit RS=P1^2;             
sbit RW=P1^1;          
sbit LCDE=P1^0;
sbit LED=P1^6;
sbit BUZZ=P1^7;
sbit INT2=P3^5;       

sbit SCL=P1^3;        
sbit SDA=P1^4;  

uchar bdata EEP;
sbit EEP_7 = EEP^7;
sbit EEP_0 = EEP^0;     

bdata uchar flag;
sbit bit_out=flag^1;
sbit bit_in=flag^0;
sbit sta=flag^2;
sbit set=flag^3;
sbit his=flag^4;
sbit sw=flag^5;
sbit setw=flag^6;
sbit setd=flag^7;

bdata uchar flag2;
sbit setm=flag2^0;
sbit bluea=flag2^1;

uchar temH,temL,err;

bit RxdOrTxd = 0; 
bit RxdEnd = 0; 
bit TxdEnd = 0; 
uchar RxdBuf = 0; 
uchar TxdBuf = 0; 
sbit PIN_RXD = P3^0; 
sbit PIN_TXD = P3^1;

void busy(void);
void cmd_w(uchar cmd);
void init1602(void);
void dat_wrt(uchar dat); 
void LCD_print(uchar *str,uchar n);

void I2C_Start(void);
void I2C_Stop(void);
void I2C_Ack(void);
void Write_Byte(uchar wdata);
uchar Read_Byte(void);
uchar eread(uchar addr);
void ewrite(uchar addr,uchar dat);

void ConfigUART(unsigned int baud);
void StartTXD(unsigned char dat);
void StartRXD();
void BlueSend(uchar *str,uchar n);
void delay(uint a);
void i2c_Init(void);
void start_bit(void);
void stop_bit(void);
void send(uchar dat_byte);
void send_bit(void);
uchar read(void);
void receive_bit(void);
uint gettemp(void);
void mtemp(void);
void hisswitch(void);
void SendHis();

uchar flag1;
uint tem;
float otem;
uint temthread;
uchar hisnum;
uchar s[10];
uchar sel;
uchar td[3];
uchar index;

void main()
{
	
	EX0 = 1;        
    IT0 = 1;                
    EX1 = 1;        
    IT1 = 1;
	EA=1; 
	sta=0;
	his=0;
	sw=0;
	setw=0;
	setd=0;
	ConfigUART(9600);
	TMOD &= 0x0F;
	TMOD |= 0x60;
	TH1=0xFF;
	TL1=0xFF;
	ET1=1;
	TR1=1;
	init1602();
	
/*	ewrite(Thread1,3);
	ewrite(Thread2,7);
	ewrite(Thread3,4);
	ewrite(Hisn,0);	   */
	td[0]=eread(Thread1);
	td[1]=eread(Thread2);
	td[2]=eread(Thread3);
	temthread=td[0]*1000+td[1]*100+td[2]*10;
	hisnum=eread(Hisn);
	
	LCD_print("    Welcome!      Press to START",1);
	
	while(1)
	{    
	/*		 */
		if(sta)
		{
			LCD_print("Result:",1);
			mtemp();
			//BlueSend(s,5);
		 	sta=0;
			LCD_print(s,0);
			if(tem>temthread){
				BUZZ=0;
				LED=0;
				delay(1000);
				BUZZ=1;
				LED=1;
			}  
			
			hisnum++;
			if(hisnum==9)
				hisnum=1;  
			
			ewrite(Hisn,hisnum);
		    ewrite(EEPDa+hisnum*3,tem/1000);  
		    ewrite(EEPDa+hisnum*3+1,tem % 1000/100);  
		 	ewrite(EEPDa+hisnum*3+2,tem%100/10); 
				
		}else
		if(set)
		{
			if(!setd)
			{
				s[0]=td[0]+0x30;
				s[1]=td[1]+0x30;
				s[3]=td[2]+0x30;
				s[2]='.';
				s[4]='C';
				s[5]=0;
				LCD_print("Set:",1);
				LCD_print(s,0);
				setd=1;
			}
			if(setw)
			{
				td[2]+=1;
				if(td[2]==10)
				{
					td[1]+=1;
					td[2]=0;
				}
				ewrite(Thread3,td[2]);
				ewrite(Thread2,td[1]);
				setd=0;
				setw=0;
			}
			if(setm)
			{
				td[2]-=1;
				if(!td[2])
				{
					td[1]-=1;
					td[2]=9;
				}
				ewrite(Thread3,td[2]);
				ewrite(Thread2,td[1]);
				setd=0;
				setm=0;
			}

		}else
		if(his)
		{	
			if(sw)
			{
				if(sel==0)
					sel=8;
				s[0]=eread(EEPDa+sel*3)+0x30;
				s[1]=eread(EEPDa+sel*3+1)+0x30;
				s[3]=eread(EEPDa+sel*3+2)+0x30;
				s[2]='.';
				s[4]='C';
				s[5]='\0';
				if(sel<=hisnum){
					index=hisnum+1-sel+0x30;
				}else{
					index=9+hisnum-sel+0x30;
				}
				sel--;
				LCD_print("History:",1);	
				LCD_print(s,0);
				s[0]=0x20;
				s[1]=0x20;
				s[2]=0x20;
				s[3]=0x20;
				s[4]=index;
				s[5]=0;
				LCD_print(s,0);
				sw=0;			   
			}
			if(bluea)
			{
				while (PIN_RXD&bluea); 
				StartRXD(); 
				while (!RxdEnd&bluea);
				
				SendHis();
				delay(5000);	
			}
		}else{
		 	
		}
	}
}

void SendHis()
{
	uchar i,j;
	j=hisnum;
	for(i=0;i<8;i++,j++)
	{
		if(j==9)
			j=1;
		s[0]=eread(EEPDa+j*3)+0x30;
		s[1]=eread(EEPDa+j*3+1)+0x30;
		s[3]=eread(EEPDa+j*3+2)+0x30;
		s[2]='.';
		s[4]='C';
		s[5]=0x20;
		s[6]=i+1+0x30;
		s[7]='\r';
		s[8]='\n';
		s[9]='\0';
		BlueSend(s,10);
	}
	//s[0]=0x20;s[1]=0x20;s[2]=0x20;s[3]=0;
	LCD_print("   ",0);
	s[0]='O';
	s[1]='K';
	s[2]='!';
	s[3]='\0';
	BlueSend(s,4);
	LCD_print(s,0);

}

void int0() interrupt 0
{
	delay(100);
	if(INT0==0)
	{
    	LED=~LED;
		if(his)
		{
			set=1;
			setd=0;
			his=0;
		}else
		if(set){
			set=0;
			his=0;
			LCD_print("    Welcome!      Press to START",1);
		}else{
		 	his=1;
		}
		if(his)
		{
			sel=hisnum;
			sw=1;
		}
	}
}

void int1() interrupt 2
{
	delay(100);
	if(INT1==0)
	{
		if(his)
		{
			sw=1;			
		}else
		if(set)
		{
			setw=1;
		}else{
			sta=1;
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
			  	bluea=0;
				sel=hisnum;
				sw=1;
			}else{
				bluea=1;
				LCD_print("Bluetooth",1);
			}				
		}else
		if(set)
		{
			setm=1;
		}else{
			
		}
	}
}

void mtemp(void)
{
	uchar i;
	tem=0;
	i2c_Init();
	for(i=0;i<10;i++)
		tem=tem+gettemp();
		delay(10);
	otem=tem/1000;
	tem=100*(otem*otem*otem*(-0.0012)+0.1719*otem*otem+125.7-7.06719*otem);
	s[0]=tem/1000+0x30;
	s[1]=tem % 1000/100+0x30;
	s[2]=0x2E;
	s[3]=tem%100/10+0x30;
	s[4]=0x43;
	s[5]='\0';
}

void delay(uint a)
{                        
    uint b;
    for(;a>0;a--)
		for(b=125;b>0;b--);
}


void i2c_Init(void)
{
	SCL=1;
	SDA=1;
	_nop_();
	_nop_();
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
	_nop_();_nop_();_nop_();_nop_();_nop_();
	SCL=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();
}

void stop_bit(void)
{
	SCL=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();
	SDA=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();
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
			bit_out=1;
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
			goto Repeat;
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
	SCL=1;
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
		dat=dat<<1;
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
	bit_in=SDA;
	_nop_();
	SCL=0;
	_nop_();_nop_();_nop_();_nop_();
	_nop_();_nop_();_nop_();_nop_();
}

uint getTemp(void)
{
	start_bit();
	send(0xB4); 
	send(0x07);  
	start_bit();
	send(0x01);
	bit_out=0;
	temL=read();
	bit_out=0;
	temH=read();
	bit_out=1;
	err=read();
	stop_bit();
	return (temH*256+temL)*2-27315;
}

void BlueSend(uchar *str,uchar n)
{
	uchar i;
	for(i=0;i<n;i++){
		StartTXD(str[i]);
		while (!TxdEnd); 
		delay(100);                                                                               
	}
}

void ConfigUART(unsigned int baud)
{
	TMOD &= 0xF0; 
	TMOD |= 0x02; 
	TH0 = 256 - (11059200/12)/baud; 
}


void StartRXD()
{
	TL0 = 256 - ((256-TH0)>>1); 
	ET0 = 1; 
	TR0 = 1; 
	RxdEnd = 0; 
	RxdOrTxd = 0; 
}

void StartTXD(unsigned char dat)
{
	TxdBuf = dat; 
	TL0 = TH0; 
	ET0 = 1; 
	TR0 = 1; 
	PIN_TXD = 0; 
	TxdEnd = 0; 
	RxdOrTxd = 1; 
}

void InterruptTimer0() interrupt 1
{
	static unsigned char cnt = 0; 
	if (RxdOrTxd) 
	{
		cnt++;
		if (cnt <= 8) 
		{
			PIN_TXD = TxdBuf & 0x01;
			TxdBuf >>= 1;
		}
		else if (cnt == 9) 
		{
			PIN_TXD = 1;
		}else{
			cnt = 0; 
			TR0 = 0; 
			TxdEnd = 1; 
		}
	}else{
		if (cnt == 0) 
		{
			if (!PIN_RXD) 
			{
				RxdBuf = 0;
				cnt++;
			}else{
				TR0 = 0; 
			}
		}else if (cnt <= 8){
			RxdBuf >>= 1; 
			if (PIN_RXD) 
			{ 
				RxdBuf |= 0x80;
			}
			cnt++;
		}else{
			cnt = 0; 
			TR0 = 0; 
			if (PIN_RXD)
			{
				RxdEnd = 1; 
			}
		}
	}
}

void cmd_w(uchar cmd)                                  
{
	//LCDE=0;
	busy();                                                 
	P2=cmd;                                                 
	RS=0;                                                    
	RW=0;                                                    
	LCDE=1;                                               
	LCDE=0;
}

void init1602(void)                            
{
	cmd_w(0x01);                                 //Clean
	cmd_w(0x0c);                                 //Inital Settings
	cmd_w(0x06);
	cmd_w(0x38);  
}
                               

void busy(void)                                 //LCD is busy ?
{
	flag1=0x80;                                    
	while(flag1&0x80)                          
	{
		P2=0xff;
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
//	LCDE=0;
	if(flag1==16)
	{	
		P2=0xC0;
	    RS=0;                                               
	    RW=0;                                                                                        
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
	{
		dat_w(*str);                                  
		str++;                                                
	}
}

void I2C_Start(void)
{
	SDA=1;
	delay(1);
	SCL=1;
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
	delay(1);
	SDA=1;
	delay(1);	
}

void I2C_Ack(void)
{
	uchar i;
	SCL=1;
	delay(1);
	while((SDA==1)&&(i<255)) i++;
	SCL=0;
	delay(1);
	SDA=1;
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
		EEP<<=1;
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
		EEP<<=1;
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
	Write_Byte(dat);
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
	I2C_Start();
	Write_Byte(0xa1);
	I2C_Ack();
	dat=Read_Byte();
	I2C_Stop();
	return dat;
}