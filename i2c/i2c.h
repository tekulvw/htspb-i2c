/*
RobotC I2C implementation for the HiTechnic Protoboard
Copyright 2014 Noah Moroze, FTC 4029 Captain

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/lgpl.txt>.

API is based on the Wire library for Arduino
http://arduino.cc/en/Reference/Wire
*/

#include "drivers/hitechnic-superpro.h"

tSensors _i2c_HTSPB;

typedef struct {
	ubyte _i2c_sda;
	ubyte _i2c_scl;
	bool _i2c_sda_state;
	bool _i2c_scl_state;
	ubyte _i2c_rxBuffer[32]; //max buffer size is 32
	ubyte _i2c_rxBufferLen;
} I2CSensor;

/**
  * Init I2CSensor struct with bufferLen = 0
  * @param this - pointer to an I2CSensor
  */
void i2c_init(I2CSensor* this);

/**
  * Initialize I2C library
  * @param this - I2C sensor to use
  * @param HTSPB - the HiTechnic Sensor Protoboard being used
  * @param sda - which digital pin (from 0-7) is being used as the SDA pin
  * @param scl - which digital pin (from 0-7) is being used as the SCL pin
  */
void i2c_begin(I2CSensor* this, tSensors HTSPB, int sda, int scl);

/**
  * Send start sequence and address of slave to begin sending data
  * @param this - I2C sensor to use
  * @param address - the 7-bit address of the slave
  * @param write - if 0, begins transmission in write mode, otherwise begins transmission in read mode (default 0)
  */
void i2c_beginTransmission(I2CSensor* this, ubyte address, ubyte write);
void i2c_beginTransmission(I2CSensor* this, ubyte address);

/**
  * Send stop sequence
  * @param this - I2C sensor to use
  */
void i2c_endTransmission(I2CSensor* this);

/**
  * Write byte to slave
  * @param this - I2C sensor to use
  * @param value - byte to write to slave
  * @return - true if ACK bit received, false if not
  */
bool i2c_write(I2CSensor* this, ubyte value);

/**
  * Write bytes to slave
  * @param this - I2C sensor to use
  * @param values - bytes to write to slave
  * @param length - how many bytes to write from the array, starting at index 0
  * @return - how many bytes received an ACK bit in response
  */
int i2c_write(I2CSensor* this, ubyte *values, int length);

/**
  * Write string to slave
  * @param this - I2C sensor to use
  * @param str - string to write to slave
  * @return - how many bytes received an ACK bit in response
  */
int i2c_write(I2CSensor* this, char *str);

/**
  * Read bytes from slave into RX buffer
  * @param this - I2C sensor to use
  * @param address - address of slave
  * @param quantity - number of bytes to request
  * @return - number of bytes received
  */
ubyte i2c_requestFrom(I2CSensor* this, ubyte address, int quantity);

/**
  * @param this - I2C sensor to use
  * @return - number of bytes in RX buffer
  */
int i2c_available(I2CSensor* this);

/**
  * @param this - I2C sensor to use
  * @return - the oldest byte in RX buffer
  */
ubyte i2c_read(I2CSensor* this);

// internal functions to handle I/O
void _i2c_sda_write(I2CSensor* this, bool val);
bool _i2c_sda_read(I2CSensor* this);
void _i2c_scl_write(I2CSensor* this, bool val);
bool _i2c_scl_read(I2CSensor* this);

void i2c_init(I2CSensor* this) {
	this->_i2c_rxBufferLen = 0;
}

void i2c_begin(I2CSensor* this, tSensors HTSPB, int sda, int scl) {
	_i2c_HTSPB = HTSPB;
	//_i2c_sda = 1 << sda;
	//_i2c_scl = 1 << scl;
	this->_i2c_sda = 1 << sda;
	this->_i2c_scl = 1 << scl;
}

void i2c_beginTransmission(I2CSensor* this, ubyte address) {
	i2c_beginTransmission(this, address, 0);
}

void i2c_beginTransmission(I2CSensor* this, ubyte address, ubyte write) {
	// write start sequence
	_i2c_sda_write(this,1);
	_i2c_scl_write(this,1);
	_i2c_sda_write(this,0);
	_i2c_scl_write(this,0);

	// write address of slave
	i2c_write(this,(address << 1) | write); // send write bit as LSB
}

void i2c_endTransmission(I2CSensor* this) {
	// write stop sequence
	_i2c_sda_write(this, 0);
	_i2c_scl_write(this, 1);
	_i2c_sda_write(this, 1);
}

bool i2c_write(I2CSensor* this, ubyte value) {
	ubyte mask;

	// iterates over bits from MSB to LSB and writes each one
	for(mask = 1 << 7; mask != 0; mask >>= 1) {
		bool b = (value & mask) != 0;
		_i2c_sda_write(this,b);
		_i2c_scl_write(this,1);
		_i2c_scl_write(this,0);
	}

	// check ACK bit
	_i2c_sda_write(this,1);
	_i2c_scl_write(this,1);
	bool ack = !_i2c_sda_read(this);
	_i2c_scl_write(this,0);
	_i2c_sda_write(this,0);
	return ack;
}

int i2c_write(I2CSensor* this, ubyte *values, int length) {
	int ack = 0;
	int i;
	for(i=0; i<length; i++) {
		if(i2c_write(this, values[i]))
			ack++;
	}

	return ack;
}

int i2c_write(I2CSensor* this, char* str) {
	int length = strlen(str);
	int ack = 0;
	int i;
	for(i=0; i<length; i++) {
		if(i2c_write(this, str[i]))
			ack++;
	}

	return ack;
}

ubyte i2c_requestFrom(I2CSensor* this, ubyte address, int quantity) {
	i2c_beginTransmission(this, address, 1);

	int i;
	for(i=0; i<quantity; i++) {
		_i2c_sda_write(this, 1); // let go of SDA line
		ubyte response = 0;
		int j;
		for(j=0; j<8; j++) {
			response <<= 1;
			_i2c_scl_write(this, 1);
			long beganReadTime = time1[T1];
			// wait for clock stretching
			while(!_i2c_scl_read(this)) {
				if(time1[T1] > beganReadTime + 1000) // timeout after 1000ms
					return i;
			}
			wait1Msec(2); // wait a bit before reading
			response |= (_i2c_sda_read(this) ? 1 : 0);
			_i2c_scl_write(this,0);
		}

		_i2c_sda_write(this,0); // send ACK bit
		_i2c_scl_write(this,1);
		_i2c_scl_write(this,0);
		this->_i2c_rxBuffer[this->_i2c_rxBufferLen] = response;
		this->_i2c_rxBufferLen++;
	}

	return quantity;
}

int i2c_available(I2CSensor* this) {
	return this->_i2c_rxBufferLen;
}

ubyte i2c_read(I2CSensor* this) {
	ubyte val = this->_i2c_rxBuffer[0]; // return last byte read
	// shift remaining bytes over one
	int i;
	for(i=0; i<this->_i2c_rxBufferLen-1; i++) {
		this->_i2c_rxBuffer[i] = this->_i2c_rxBuffer[i+1];
	}
	this->_i2c_rxBufferLen--;

	return val;
}

void _i2c_sda_write(I2CSensor* this, bool val) {
	this->_i2c_sda_state = val;
	ubyte mask =
		(this->_i2c_sda_state ? 0:1) * this->_i2c_sda |
		(this->_i2c_scl_state ? 0:1) * this->_i2c_scl;
	HTSPBsetupIO(_i2c_HTSPB, mask);
}

void _i2c_scl_write(I2CSensor* this, bool val) {
	this->_i2c_scl_state = val;
	ubyte mask =
		(this->_i2c_sda_state ? 0:1) * this->_i2c_sda |
		(this->_i2c_scl_state ? 0:1) * this->_i2c_scl;
	HTSPBsetupIO(_i2c_HTSPB, mask);
}

bool _i2c_sda_read(I2CSensor* this) {
	return HTSPBreadIO(_i2c_HTSPB, this->_i2c_sda) != 0;
}

bool _i2c_scl_read(I2CSensor* this) {
	return HTSPBreadIO(_i2c_HTSPB, this->_i2c_scl) != 0;
}
