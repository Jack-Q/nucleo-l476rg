/******************************************************
 * Max7219 displayer
 * -----------------
 * Convert function GIO_init() and max7219_send() to
 * the format confronting the AAPCS, and implement
 * the function display() in C language, which can
 * display student number on 7-seg via max7219_send().
 ******************************************************/

extern void GPIO_init();
extern void Max7219_init();
extern void Max7219_send(unsigned char address, unsigned char data);

#define MAX_SCAN_LIM 0x0B

/**
 * Show data on 7-seg via max7219_send
 * Input:
 *   data: decimal value
 *   num_digs: number of digits will show on 7-seg
 * Return:
 *   0: success
 *   -1: illegal data range (out of 8 digits range)
 */
int display(int data, int num_digs) {
  if (data > 99999999 || data < -9999999) return -1;
  if (num_digs > 8 || num_digs <= 0) return -1;

  Max7219_send(MAX_SCAN_LIM, num_digs - 1);
  register unsigned int num = 0;
  register int tmp;
  while (num_digs--) {
    tmp = data / 10;
    Max7219_send(++num, data - tmp * 10);
    data = tmp;
  }
  return data != 0;
}

int main() {
  int student_id = 540017;
  GPIO_init();
  Max7219_init();
  display(student_id, 7);

  return 0;
}
