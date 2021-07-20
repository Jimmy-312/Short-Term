#include <reg52.h>
#include<intrins.h>
#define uint unsigned int
#define uchar unsigned char

sbit RES=P0^0;
sbit LCDE=P1^0;
sbit RW=P1^1;
sbit RS=P1^2;
sbit SCL=P1^3;
sbit SDA=P1^4;
sbit SA=P1^5;
sbit LED=P1^6;
sbit BUZZ=P1^7;

void delay(uint i);
void send_bit(uchar dat);
uchar read_bit(void);
uchar ReadByte(void);
void start(void);
void stop(void);
uint getTemp(void);
void busy(void);
void cmd_w(uchar cmd);
void init1602(void);

void main()
{
	//send_bit(1);
}

void delay(uint n)
{
	uint i;
	for(i=0;i<n;i++)
	{
		_nop_();
	}
}

void send_bit(uchar dat)
{
	SDA=dat;                                                                         
	_nop_();
	SCL=1;                                                           
	delay(8);
	SCL=0;
	delay(8);
}

uchar read_bit(void)
{
	uchar rdata;
	SDA=1;                                                 
	SCL=1;                                                   
	delay(8);
	rdata=SDA;                                             
	_nop_();
	SCL=0;
	delay(8);
	return rdata;
}

void SendByte(uchar dat)
{
    uchar i,n=2,rdata;                                               
	Send:
	for(i=0;i<8;i++)                                       
	{                                  
		send_bit(dat&0x80);                                            
		dat=dat<<1;                                            
	}
	rdata=read_bit();                                                  
	if(rdata==1)                                          
	{
		stop();
		if(n!=0)
		{
			n--;                                            
			goto Repeat;                                     
		}  
    }
	goto Exit;
	Repeat:
	start();                                                      
	goto Send;                                              
	Exit: ;                                                        
}

uchar ReadByte(void)
{
	uchar i,dat,rbit;
	dat=0;                                                        
	for(i=0;i<8;i++)
	{
		dat=dat<<1;                                                  
		rbit=read_bit();                                                 
		dat+=rbit;                                           
	}                                                   
	send_bit(0);
	return dat;                                              
}

void start(void)                                        
{
	SDA=1;
	delay(4);
	SCL=1;
	delay(4);
	SDA=0;
	delay(4);
	SCL=0;
	delay(4);
}

void stop(void)                                               
{
	SCL=0;
	delay(4);
	SDA=0;
	delay(4);
	SCL=1;
	delay(4);
	SDA=1;
}

uint getTemp(void)
{
	uchar tempL,tempH,error;
	SCL=0;
	start();
	SendByte(0x00);	//Send address, default:0x00;
	SendByte(0x07);
	start();
	SendByte(0x01);
	tempL=ReadByte();
	tempH=ReadByte();
	error=ReadByte();
	stop();
	return tempL+tempH*256;
}

void cmd_w(uchar cmd)                                  
{
	LCDE=0;
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
	uchar flag=0x80;                                    
	while(flag&0x80)                          
	{
		P1=0xff;
		RS=0;                                               
		RW=1;                                             
		LCDE=1;                                       
		flag=P1;                                       
		LCDE=0;
	}
}