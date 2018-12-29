#include "SSD1306.h"
#include <cstring>
I2C_HandleTypeDef *_i2c; //����� �� ��������� I2C
uint16_t _address; //����� �������
uint8_t _x, _y; //������� ���������� X � Y
OLED_font *_font = &Lucida_Console_12; //������ �� ������� �����. �� ��������� Lucida Console 12 ����
uint8_t displayBuffer[8][128]; //����� ������� 8 ������� �� 128 ��������

/* ���������������� ������� */		

//������� ������������� ������� 	
uint8_t OLED_init(I2C_HandleTypeDef *i2c, uint16_t address) {
	_i2c = i2c;
	_address = address;
	
	//�������� ��������� ���������� �� ����
	uint8_t STATUS = HAL_I2C_IsDeviceReady(_i2c, _address, 5, 0xFF);
	//���� ���������� �� ������ - ������������ � �������
	if (STATUS != HAL_OK) {
		return STATUS;
	}
	//1. Set Display Off
	//__sendCommand(0xAE);
	//2. Set Osc Frequency
	__sendCommandWithData(0xD5, 0xF0); //Max
	//3. Set MUX Ratio (0xA8)
	__sendCommandWithData(0xA8,0x3F);
	//4. Set Display Offset
	__sendCommandWithData(0xD3, 0x00);
	//5. Set Display Start Line
	__sendCommand(0x40);
	//6. Enable charge pump regulator
	__sendCommandWithData(0x8D, 0x14);
	//7. Set Segment re-map. 0xA1 - left-to-right, 0xA0 - right-to-left
	__sendCommand(0xA1);
	//8. Set COM Output Scan Direction 0xC0 - down-to-up, 0xC8 - up-to-down
	__sendCommand(0xC8);
	//9. Set COM Pins hardware configuration
	__sendCommandWithData(0xDA, 0x12);
	//10. Set Contrast Control
	__sendCommandWithData(0x81, 0x7F);//127, half-brightness
	//11. Set Pre-Charge period
	__sendCommandWithData(0xD9, 0x22);
	//12. Set VCOMH Deselect Level
	__sendCommandWithData(0xDB, 0x30);
	//13. Disable Entire Display On
	__sendCommand(0xA4);
	//14. Set Normal Display
	__sendCommand(0xA6);
	//15. Clear Screen
	//16. Display On
	__sendCommand(0xAF);
	
	//��������� ����� �������������� ���������
	__sendCommandWithData(0x20, 0x00);

	//end init

	return STATUS;
}

//������� ��������� ������� �������
void OLED_setBrightness(uint8_t brightness) {
	__sendCommandWithData(0x81, brightness);
}
//��������/��������� �������
void OLED_display(uint8_t state) {
	__sendCommand(0xAE | state);
}
//��������/��������� ��������
void OLED_inversion(uint8_t state) {
	__sendCommand(0xA6 | state);
}

//������� ������� ������
void OLED_clear(void) {
	memset(displayBuffer, 0x00, 1024);
}
//������� ���������� ������
void OLED_fill(void) {
	memset(displayBuffer, 0xFF, 1024);
}
//������� ����������� �������
void OLED_goto(uint8_t x, uint8_t y) {
	_x = x;
	_y = y;
}
//������� ��������� ������� �� ������
//TODO: ���������� �������
//TODO: ��������� ��������� �������������� ����� ����� ������ ����������� �� ������ 8
void OLED_draw(const uint8_t bitmap[], uint8_t length, uint8_t width, uint8_t inversion, uint8_t transparent) {
	uint8_t _length, _width; //�������� �������� ������� ������������� ������� � ���������� �� ��, ������� ���������� ������� �� �����
	//�������� � ��������� ����� � ������ ������� 
	if ((length + _x) > (DISPLAY_LENGHT-1)) _length = DISPLAY_LENGHT-1-_x; else _length = length;
	if ((width + _y) > (DISPLAY_WIDTH-1)) _width = DISPLAY_WIDTH-1-_y; else _width = width;
	//�������� ����� ����������������� ������ �� ���� �� ��������� �����������
	if (((_length == 0) || (_length > DISPLAY_LENGHT-1)) || (_width <= 0 || (_width > DISPLAY_WIDTH-1))) return;
	//���������� ���������� �������, ������� ������ �������
	uint8_t pages = _width/8;
	if (_width%8 > 0) pages++; //���� �������� ������ �� ������ 8, �� ���-�� ������� ������������� �� 1
	//������ � ����� ��������
	for (uint8_t page = 0; page < pages; page++) {
		for (uint8_t column = 0; column < _length; column++) {
			uint8_t bufferColumn = (displayBuffer[_y/8][_x]); //������� �� ������
			uint8_t bitmapColumn = bitmap[column+page*length]; //������� �� �������
			if (inversion == ON) bitmapColumn ^= 0xFF; //��������������

			if (inversion == AUTO) {
				bufferColumn ^= (bitmapColumn << (_y%8)); //������������ � �������� ���� ������� ��� ������� �� Y
			} else {
				if (transparent == OFF) bufferColumn &= (0xFF >> (8 - _y%8)); //������� ������� ������ ���� �� �������� ������������
				bufferColumn |= (bitmapColumn << (_y%8));
			}
			displayBuffer[_y/8][_x] = bufferColumn;
			
			//������������� ������ ����� ��������, ���� ��� ���������� �� ����� � ���� �� ����� ���� �������� �������
			if ((_y+8 <= 63) & (_y%8 != 0)) {
				bufferColumn = (displayBuffer[_y/8+1][_x]); //������� �� ������ � ��������� ��������
				bitmapColumn = bitmap[column+page*length]; //������� �� �������
				if (inversion == ON) bitmapColumn ^= 0xFF; //��������������
				
				if (inversion == AUTO) {
					bufferColumn ^= (bitmapColumn >> (8-_y%8)); //������������ � �������� ���� ������� ��� ������� �� Y
				} else {
					if (transparent == OFF) bufferColumn &= (0xFF << (_y%8)); //������� ������� ������ ���� �� �������� ������������
					bufferColumn |= (bitmapColumn >> (8-_y%8));
				}
				displayBuffer[_y/8+1][_x] = bufferColumn;
			}
			_x++;
		}
		//���������� Y ����� ������ ������� �������
		_y += 8;
		//���������� X � ��������� ��������� ��� ���������� ������ ��������� �������
		_x -= _length;
	}
	//��������� X � Y ��� ���������� ������ ���������� �������
	_y -= 8*pages;
	_x += _length;
}

//������� �������� ������ �� �������
void OLED_update(void) {
	__displayUpdate();
}
//������� ������ ������� �� �������
void OLED_printChar(unsigned char byte, uint8_t inversion, uint8_t transparent) {
	//TODO: ���������� �������
	//������ �������� ��� �������� �� ASCII
	if ((byte < 128) & (byte > 31)) {
		//��������� ������� ������ �� ���������
		uint8_t font_lenght = _font -> lenght;
		uint8_t font_width = _font -> width;
		uint8_t bitmapLength = _font -> international[(byte-32)*(font_lenght*2+1)]; //������ �������
		uint8_t pages = font_width/8;
		if (font_width%8 > 0) pages++;
		
		uint8_t charBitmap[bitmapLength*pages];

		for (uint8_t page = 0; page < pages; page++) {
			for (uint8_t i = 0; i < bitmapLength; i++) {
				uint8_t bitmapIndex = page*bitmapLength+i;
				uint8_t fontIndex = 1+i*2+page;
				charBitmap[bitmapIndex] = _font -> international[(byte-32)*(font_lenght*2+1)+fontIndex];
			}
		}
		//����������� �� ��������� �������
		if (_x+bitmapLength >= DISPLAY_LENGHT) {
			_x = 0;
			_y += font_width;
		}
		
		OLED_draw(charBitmap, bitmapLength, font_width, inversion, transparent);
	}
	//������ ���������
	if (byte >= 192) {
		uint8_t font_lenght = _font -> lenght;
		uint8_t font_width = _font -> width;
		uint8_t bitmapLength = _font -> cyrillic[(byte-192)*(font_lenght*2+1)]; //������ �������
		uint8_t pages = font_width/8;
		if (font_width%8 > 0) pages++;
		
		uint8_t charBitmap[bitmapLength*pages];
		
		for (uint8_t page = 0; page < pages; page++) {
			for (uint8_t i = 0; i < bitmapLength; i++) {
				uint8_t bitmapIndex = page*bitmapLength+i;
				uint8_t fontIndex = 1+i*2+page;
				charBitmap[bitmapIndex] = _font -> cyrillic[(byte-192)*(font_lenght*2+1)+fontIndex];
			}
		}
		//����������� �� ��������� �������
		if (_x+bitmapLength >= DISPLAY_LENGHT) {
			_x = 0;
			_y += font_width;
		}
		OLED_draw(charBitmap, bitmapLength, font_width, inversion, transparent);
	}
}
//������� ������ ������ �� ������
void OLED_print(char *text, uint8_t inversion, uint8_t transparent) {
	while(*text) {
		OLED_printChar(*text++, inversion, transparent);
	}		
}
//������� ����� ������
void OLED_setFont(OLED_font *font) {
	_font = font;
}
/* ��������� ������� */

//������� ��� ������ ������ �� ������
static void __displayUpdate(void) {
	//��������� ���������� � ��������� �������
	uint8_t columns[6] = {0x80, 0x21, 0x80, 0, 0x80, 127};
	__transmitToDisplay(columns, 6);
	uint8_t pages[6] = {0x80, 0x22, 0x80, 0, 0x80, 7};
	__transmitToDisplay(pages, 6);
	for(uint8_t i = 0; i < 8; i++) {
		__sendDataArray(displayBuffer[i], 128);
	}
}
//������� �������� ������ ����� ������
static uint8_t __sendData(uint8_t byte) {
	uint8_t i2c_buffer[2];
	i2c_buffer[0] = 0xC0;//we sending data
	i2c_buffer[1] = byte;
	return __transmitToDisplay(i2c_buffer, 2);
}
//������� �������� ������� ������
static uint8_t __sendDataArray(uint8_t bytes[], uint16_t size) {
	uint8_t i2c_buffer[size+1];
	i2c_buffer[0] = 0x40; //�� ���������� ������ ������
	for (uint16_t i = 1; i <= size; i++) {
		i2c_buffer[i] = bytes[i-1];
	}
	return __transmitToDisplay(i2c_buffer, size+1);
}
//������� �������� �������
static uint8_t __sendCommand(uint8_t cmd) {
	uint8_t i2c_buffer[2];
	i2c_buffer[0] = 0x80; //we sending command
	i2c_buffer[1] = cmd;
	return 	__transmitToDisplay(i2c_buffer, 2);
}
//������� �������� ������� � �������
static uint8_t __sendCommandWithData(uint8_t cmd, uint8_t data) {
	uint8_t i2c_buffer[4];
	i2c_buffer[0] = 0x80; //we sending command
	i2c_buffer[1] = cmd;
	i2c_buffer[2] = 0x80; //we sending data after command
	i2c_buffer[3] = data;
	return __transmitToDisplay(i2c_buffer, 4);
}
//������� �������� ������ �� ���� I2C
static uint8_t __transmitToDisplay(uint8_t buff[], uint16_t size) {
	return HAL_I2C_Master_Transmit(_i2c, _address, buff, size, 0xFF);
}
