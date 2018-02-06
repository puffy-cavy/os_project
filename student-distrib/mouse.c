#include "mouse.h"
#include "lib.h"
#include "i8259.h"


#define X_SCALE 4
#define Y_SCALE 8
#define x_coor 320
#define y_coor 200
#define RAISE 1
#define NO_SCROLL 0
//struct of mouse cursor
typedef struct {
    int32_t x;
    int32_t y;
} position_t;
//Helper function declaration
static position_t mouse_pos;
uint8_t receive_command();
void send_command(uint8_t command, uint8_t port);
void mouse_wait(uint8_t type);
void mouse_move(int32_t deltaX,int32_t deltaY);

/* send_command(uint8_t command,uint8_t port)
 * send data to mouse
 *
 * Inputs: command, port
 * Outputs: None
   Code modified from OSDEV website
   */
void send_command(uint8_t command,uint8_t port){
  mouse_wait(1);
  outb(0xD4,0x64);
  mouse_wait(1);
  outb(command,port);
}
/* receive_command()
 * receive data from mouse
 *
 * Inputs: None
 * Outputs: uint8_t return value from port 0x60
   Code modified from OSDEV website
   
   */
uint8_t receive_command()
{
//Get response from mouse
  mouse_wait(0);
  return inb(0x60);
}
/* mouse_wait()
 * Wait for mouse be ready to send or receive data
 *
 * Inputs: type
 * Outputs: None
   Code modified from OSDEV website
   https://houbysoft.com/download/ps2mouse.html
   */
void mouse_wait(uint8_t type)
{
  unsigned int _time_out=100000;
  if(type==0)
  {
    while(_time_out--) //Data
    {
      if((inb(0x64) & 1)==1)
      {
        return;
      }
    }
    return;
  }
  else
  {
    while(_time_out--) //Signal
    {
      if((inb(0x64) & 2)==0)
      {
        return;
      }
    }
    return;
  }
}
/* mouse_init()
 * Initialize the mouse, Enables Keyboard IRQs
 *
 * Inputs: None
 * Outputs: None
   Code modified from OSDEV website
   https://houbysoft.com/download/ps2mouse.html
 */
void mouse_init(){
  //Reset Mouse
  send_command(0xA8,0x64);
  //get mouse Status
  send_command(0x20,0x64);
  uint8_t mouse_status=receive_command();
  mouse_status|=0x2;
  //send back modified Status to enable IRQ in mouse
  mouse_wait(1);
  outb(0x60,0x64);
  mouse_wait(1);
  outb(mouse_status,0x60);
  //Initiate cursor position and click stucture, and mouse scrolling flags
  mouse_pos.x = 0;
  mouse_pos.y = 0;
  prev_pos.x = 0;
  prev_pos.y = 1;
  int index;
  for(index=0;index<30;index++){
	  right_clicks[index].status=0;
	  left_clicks[index].status=0;
  }
  scroll_up=NO_SCROLL;
  scroll_down=NO_SCROLL;
  //enable acks
  send_command(0xF4, 0x60);
  //enable irq
  enable_irq(12);
}

/* mouse_irq_handler()
 * Handler for a mouse Interrupt
 *
 * Inputs: None
 * Outputs: None
 */
void mouse_irq_handler(/* arguments */) {
  /* code */
	cli();
  uint32_t flag=receive_command();
  /*check ack signal and reset cycle*/
  
  if(flag==0xFA){
    //check overflow
  }
  else{
    if(((flag&0x80) == 0) && ((flag&0x40)==0)){
      //get deltax and deltay, reverse sign if negative
      int32_t deltaX=receive_command();
      if (flag&0x10){
        deltaX|=0xFFFFFF00;
      }
      int32_t deltaY=receive_command();
      if(flag&0x20){
        deltaY|=0xFFFFFF00;

      }
	  //call function to change cursor position
      mouse_move(deltaX,deltaY);
	  //if detect left button clicked, find a empty space in array and assign color in video mem
	  if(flag&0x01){
		  int i;
		  for(i=0;i<30;i++){
			  if(left_clicks[i].status!=0){
				  if(((mouse_pos.x/X_SCALE)==left_clicks[i].x_char) &&((mouse_pos.y/Y_SCALE)==left_clicks[i].y_char)){
					  left_clicks[i].status=0;
					  break;
				  }
				  
				  continue;
			  }
			  else{
				left_clicks[i].status=1;
				left_clicks[i].x_char=mouse_pos.x/X_SCALE;
				left_clicks[i].y_char=mouse_pos.y/Y_SCALE;
				//left_clicks[i].x_char=prev_pos.x;
				//left_clicks[i].y_char=prev_pos.y;
				break;
			  }
		  }
	  }
	  //if detect right button clicked, find a empty space in array and assign color in video mem
	  if(flag&0x02){
		  int i;
		  for(i=0;i<30;i++){
			  if(right_clicks[i].status!=0){
				  if(((mouse_pos.x/X_SCALE)==right_clicks[i].x_char) &&((mouse_pos.y/Y_SCALE)==right_clicks[i].y_char)){
					  right_clicks[i].status=0;
					  break;
				  }
				  
				  continue;
			  }
			  else{
				right_clicks[i].status=1;
				right_clicks[i].x_char=mouse_pos.x/X_SCALE;
				right_clicks[i].y_char=mouse_pos.y/Y_SCALE;
				break;
				//right_clicks[i].x_char=prev_pos.x;
				//right_clicks[i].y_char=prev_pos.y;
			  }
		  }
	  }
    }
  }
	
  send_eoi(12);
  sti();
  
  
}
/* mouse_move(int32_t deltaX,int32_t deltaY)
 * Handle mouse move situation with input moving value
 *
 * Inputs: deltaX, deltaY
 * Outputs: None
 */
void mouse_move(int32_t deltaX,int32_t deltaY){
  //scale mouse position
  prev_pos.x=mouse_pos.x/X_SCALE;
  prev_pos.y=mouse_pos.y/Y_SCALE;
  //set_screen
  mouse_pos.x += deltaX;
  mouse_pos.y -= deltaY;//since delta down to be negative
  //make sure mouse do not go out of range
  if(mouse_pos.x<0){
    mouse_pos.x=0;
  }
  else if(mouse_pos.x>=x_coor){
    mouse_pos.x=x_coor-1;
  }
  if(mouse_pos.y<0){
    mouse_pos.y=0;
	scroll_up=RAISE;
	return;
  }
  else if(mouse_pos.y>=y_coor){
    mouse_pos.y=y_coor-1;
	scroll_down=RAISE;
	return;
  }
  scroll_down=NO_SCROLL;
  scroll_up=NO_SCROLL;

}
