#include <iostream>
#include <stdio.h>
#include <Windows.h>

using namespace std;

/*
�㷨��
1������ͼ��
2�����ò�ֵ��������ÿ�����ص��ӳ��
3�������ͼ��
*/

int src_width, src_height, dst_width, dst_height, dst_lineByte, src_lineByte;
float scale = 1.5; //����ϵ��
unsigned char** bmpBuf;   //ͼ������
unsigned char** output_bilinear;     //˫���Բ�ֵͼ������
unsigned char** output_nearest;     //���ڽ���ֵͼ������
unsigned char** output_bicubic;     //˫���β�ֵͼ������
RGBQUAD ColorTable[256];           //��ɫ��


void loadImg(FILE *);              //���ļ�����ͼ��
void nearest();                    //���ڽ���ֵ
void bilinear();                   //˫���Բ�ֵ
void getW(double* w, double x);    //˫����Ȩ��
void bicubic();                    //˫���β�ֵ
void writeImg(char*path,int height,int width,RGBQUAD*ColorTable,unsigned char**data);                   //���ͼ��

int main()
{
	const char* bmpName = "lena512.bmp";
	FILE* f = fopen(bmpName, "rb");
	if (f == NULL)
	{
		cout << "Cannot open the file!" << endl;
		return -1;
	}
	loadImg(f);

	nearest();
	char outNearest[] = "nearest.bmp";
	writeImg(outNearest, dst_height, dst_width, ColorTable, output_nearest);

	bilinear();
	char outBilinear[] = "bilinear.bmp";
	writeImg(outBilinear, dst_height, dst_width, ColorTable,output_bilinear);

	bicubic();
	char outBicubic[] = "bicubic.bmp";
	writeImg(outBicubic, dst_height, dst_width, ColorTable, output_bicubic);

	return 0;
}

void loadImg(FILE* f)
{
	BITMAPINFOHEADER src_info;
	//����FILEͷֱ�ӿ�INFOͷ
	/*����������origin�趨���ļ������￪ʼƫ��,����ȡֵΪ��SEEK_CUR�� SEEK_END �� SEEK_SET
	SEEK_SET�� �ļ���ͷ
	SEEK_CUR�� ��ǰλ��
	SEEK_END�� �ļ���β
	*/
	fseek(f, sizeof(BITMAPFILEHEADER), SEEK_SET);
	fread(&src_info, sizeof(BITMAPINFOHEADER), 1, f);
	src_width = src_info.biWidth;
	src_height = src_info.biHeight;
	
	dst_height = src_height * scale;
	dst_width = src_width * scale;
	
	//��bmp�ļ���ͼ��Ŀ������4�ı���
	dst_lineByte = (dst_width + 3) / 4 * 4;
	src_lineByte = (src_width + 3) / 4 * 4;

	//����ɫ�� bitCountΪ8��һ��256����ɫ
	fread(&ColorTable, sizeof(RGBQUAD), 256, f);

	bmpBuf = new unsigned char*[src_height];
	for (int i = 0; i < src_height; i++)
		bmpBuf[i] = new unsigned char[src_width];

	for (int i = 0; i < src_height; i++)
	{
		for (int j = 0; j < src_width; j++)
		{
			fread(&bmpBuf[i][j], 1, 1, f);
		}
		fseek(f, src_lineByte - src_width, SEEK_CUR);
	}
}

void writeImg(char* path, int height, int width, RGBQUAD* ColorTable, unsigned char** data)
{
	int lineByte = (width + 3) / 4 * 4;
	FILE* output = fopen(path, "wb");
	if (output == NULL)
	{
		cout << "Cannot open the file!" << endl;
	}

	//�����ļ�ͷ
	BITMAPFILEHEADER outFileHeader;
	outFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
	outFileHeader.bfReserved1 = 0;
	outFileHeader.bfReserved2 = 0;
	outFileHeader.bfType = 0x4D42;
	outFileHeader.bfSize = outFileHeader.bfOffBits + height * lineByte;

	BITMAPINFOHEADER outInfoHeader;
	outInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	outInfoHeader.biWidth = width;
	outInfoHeader.biHeight = height;
	outInfoHeader.biPlanes = 1;
	outInfoHeader.biBitCount = 8;
	outInfoHeader.biCompression = BI_RGB;
	outInfoHeader.biXPelsPerMeter = 0;
	outInfoHeader.biYPelsPerMeter = 0;
	outInfoHeader.biSizeImage = 0;
	outInfoHeader.biClrUsed = 0;
	outInfoHeader.biClrImportant = 0;

	//д�ļ�ͷ
	fwrite(&outFileHeader, sizeof(BITMAPFILEHEADER), 1, output);
	fwrite(&outInfoHeader, sizeof(BITMAPINFOHEADER), 1, output);

	//д��ɫ��
	for (int i = 0; i < 256; i++)
	{
		fwrite(&ColorTable[i], 1, 4, output);
	}

	int zero = 0x00;
	//дͼ������
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			fwrite(&data[i][j], 1, 1, output);
		}
		for (int j = width; j < lineByte; j++)
		{
			fwrite(&zero, 1, 1, output);
		}
	}

	fclose(output);
}

void nearest()
{
	//��ʼ�����
	output_nearest = new unsigned char* [dst_height];
	for (int i = 0; i < dst_height; i++)
		output_nearest[i] = new unsigned char[dst_width];

	int x, y;
	for (int i = 0; i < dst_height; i++)
	{
		for (int j = 0; j < dst_width; j++)
		{
			x = i / scale;
			y = j / scale;
			output_nearest[i][j] = bmpBuf[x][y];
		}
	}
}
void bilinear()
{
	//��ʼ�����
	output_bilinear = new unsigned char* [dst_height];
	for (int i = 0; i < dst_height; i++)
		output_bilinear[i] = new unsigned char[dst_width];
	//���в�ֵ
	int x, y; //��Ӧ��ԭͼ�е�����
	unsigned char f00, f01, f10, f11;
	float u, v;

	for (int i = 0; i < dst_height; i++)
	{
		x = int((i + 0.5) / scale - 0.5);
		for (int j = 0; j < dst_width; j++)
		{
			y = int((j + 0.5) / scale - 0.5);
			//�߽������ڽ���ֵ
			if ((int)x < src_height - 1 && (int)y < src_width - 1)
			{
				f00 = bmpBuf[x][y];
				f01 = bmpBuf[x][y + 1];
				f10 = bmpBuf[x + 1][y];
				f11 = bmpBuf[x + 1][y + 1];

				u = (i + 0.5) / scale - 0.5 - x;
				v = (j + 0.5) / scale - 0.5 - y;
				output_bilinear[i][j] = f00 * (1 - u) * (1 - v) + f10 * u * (1 - v) + f01 * (1 - u) * v + f11 * u * v;
			}
			else
			{
				output_bilinear[i][j] = bmpBuf[x][y];
			}
		}
	}
}

void getW(double* w, double x)
{
	int round = (int)x;
	//˳��S(u+1) S(u) S(u-1) S(u-2)
	double temp[4] = { 1 + (x - round), x - round, x - round - 1, x - round - 2 };
	double a = 0.5;
	//������ʽ�����ӦW
	w[0] = a * abs(temp[0] * temp[0] * temp[0]) - 5 * a * temp[0] * temp[0] + 8 * a * abs(temp[0]) - 4 * a;
	w[1] = (a + 2) * abs(temp[1] * temp[1] * temp[1]) - (a + 3) * temp[1] * temp[1] + 1;
	w[2] = (a + 2) * abs(temp[2] * temp[2] * temp[2]) - (a + 3) * temp[2] * temp[2] + 1;
	w[3] = a * abs(temp[3] * temp[3] * temp[3]) - 5 * a * temp[3] * temp[3] + 8 * a * abs(temp[3]) - 4 * a;
}

void bicubic()
{
	//��ʼ�����
	output_bicubic = new unsigned char* [dst_height];
	for (int i = 0; i < dst_height; i++)
		output_bicubic[i] = new unsigned char[dst_width];
	
	double x, y;
	double w_x[4], w_y[4];
	//������Ȧ�����ڽ� ����������˫���β�ֵ
	for (int i = 0; i < dst_height; i++)
	{

		x = (i / scale);
		for (int j = 0; j < dst_width; j++)
		{
			y = (j / scale);

			if ((int)x > 2 && (int)x < src_height - 2 && (int)y > 2 && (int)y < src_width - 2)
			{
				getW(w_x, x);
				getW(w_y, y);

				double temp_pix = 0;
				for (int ii = 0; ii < 4; ii++)
				{
					for (int jj = 0; jj < 4; jj++)
					{
						temp_pix += bmpBuf[(int)x - 2 + ii][(int)y - 2 + jj] * w_x[ii] * w_y[jj];
					}
				}
				output_bicubic[i][j] = (int)temp_pix;
			}
			else
			{
				output_bicubic[i][j] = bmpBuf[int(i / scale)][int(j / scale)];
			}
		}
	}
}