#pragma once
#ifndef SEI_PACKET_H
#define SEI_PACKET_H

#include <stdint.h>

//FFMPEG的UUID
//static unsigned char FFMPEG_UUID[] = { 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef };
//时间戳UUID
static uint8_t TIME_STAMP_UUID[] = { 0x54, 0x80, 0x83, 0x97, 0xf0, 0x23, 0x47, 0x4b, 0xb7, 0xf7, 0x4f, 0x32, 0xb5, 0x4e, 0x06, 0xac };
//IMU数据UUID
static uint8_t IMU_UUID[] = { 0x94,0x51,0xef,0x8f, 0xd2,0x41, 0x49,0x6a, 0x80, 0xba, 0x68, 0x18, 0xe2, 0x4d, 0xc0, 0x4e };

#define UUID_SIZE 16

//大小端转换
uint32_t reversebytes(uint32_t value);

//检查是否为标准H264
bool check_is_annexb(uint8_t * packet, int32_t size);

//获取H264类型
int get_annexb_type(uint8_t * packet, int32_t size);

//获取sei包长度
int32_t get_sei_packet_size(const uint8_t *data, int32_t size,int annexbType);

//填充sei数据
int32_t fill_sei_packet(uint8_t * packet, int annexbType, const uint8_t *uuid, const uint8_t * content, int32_t size);

//获取标准H264 sei内容
int get_annexb_sei_content(uint8_t * packet, int32_t size, const uint8_t *uuid, uint8_t ** pdata, int32_t *psize);

//获取非标准H264 sei内容
int get_mp4_sei_content(uint8_t * packet, int32_t size, const uint8_t *uuid, uint8_t ** pdata, int32_t *psize);

//获取sei内容
int32_t get_sei_content(uint8_t * packet, int32_t size, const uint8_t *uuid, uint8_t ** pdata, int32_t *psize);

//释放sei内容缓冲区
void free_sei_content(uint8_t**pdata);

#endif