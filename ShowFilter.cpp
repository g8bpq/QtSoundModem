/*
Copyright (C) 2019-2020 Andrei Kopanchuk UZ7HO

This file is part of QtSoundModem

QtSoundModem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QtSoundModem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QtSoundModem.  If not, see http://www.gnu.org/licenses

*/

// UZ7HO Soundmodem Port by John Wiseman G8BPQ

#include "UZ7HOStuff.h"
#include <QPainter>

// This displays a graph of the filter characteristics

#define c3 -1.5000000000000E+00f //  cos(2*pi / 3) - 1;
#define c32 8.6602540378444E-01f //  sin(2*pi / 3);

#define u5 1.2566370614359E+00f //  2*pi / 5;
#define c51 -1.2500000000000E+00f // (cos(u5) + cos(2*u5))/2 - 1;
#define c52 5.5901699437495E-01f // (cos(u5) - cos(2*u5))/2;
#define c53 -9.5105651629515E-0f //- sin(u5);
#define c54 -1.5388417685876E+00f //-(sin(u5) + sin(2*u5));
#define c55 3.6327126400268E-01f // (sin(u5) - sin(2*u5));
#define c8 = 7.0710678118655E-01f //  1 / sqrt(2);


float pnt_graph_buf[4096];
float graph_buf[4096];
float prev_graph_buf[4096];
float src_graph_buf[4096];
float graph_f;
float RealOut[4096];
short RealIn[4096];
float ImagOut[4096];

#define Image1Width 642
#define Image1Height 312

void filter_grid(QPainter * Painter)
{
	int col = 20;
	int row = 8;
	int top_margin = 10;
	int bottom_margin = 20;
	int left_margin = 30;
	int right_margin = 10;

	int x, y;
	float kx, ky;

	QPen pen;  // creates a default pen

	pen.setStyle(Qt::DotLine);
	Painter->setPen(pen);


	ky = 35;

	kx = (Image1Width - left_margin - right_margin - 2) / col;

	for (y = 0; y < row; y++)
	{
		Painter->drawLine(
			left_margin + 1,
			top_margin + round(ky*y) + 1,
			Image1Width - right_margin - 1,
			top_margin + round(ky*y) + 1);
	}

	for (x = 0; x < col; x++)
	{
		Painter->drawLine(
			left_margin + round(kx*x) + 1,
			top_margin + 1,
			left_margin + round(kx*x) + 1,
			Image1Height - bottom_margin - 1);
	}

	pen.setStyle(Qt::SolidLine);
	Painter->setPen(pen);

	for (y = 0; y < row / 2; y++)
	{
		char Textxx[20];

		sprintf(Textxx, "%d", y * -20);

		Painter->drawLine(
	 		left_margin + 1,
			top_margin + round(ky*y * 2) + 1,
			Image1Width - right_margin - 1,
			top_margin + round(ky*y * 2) + 1);
		
		Painter->drawText(
			1,
			top_margin + round(ky*y * 2) + 1,
			100, 20, 0, Textxx);

	}


	for (x = 0; x <= col / 5; x++)
	{
		char Textxx[20];

		sprintf(Textxx, "%d", x * 1000);

		Painter->drawLine(
			left_margin + round(kx*x * 5) + 1,
			top_margin + 1,
			left_margin + round(kx*x * 5) + 1,
			Image1Height - bottom_margin - 1);

		Painter->drawText(
			top_margin + round(kx*x * 5) + 8,
			Image1Height - 15,
			100, 20, 0, Textxx);
	}
}

extern  "C" void FourierTransform(int NumSamples, short * RealIn, float * RealOut, float * ImagOut, int InverseTransform);


void make_graph(float * buf, int buflen, QPainter * Painter)
{
	int top_margin = 10;
	int bottom_margin = 20;
	int left_margin = 30;

	int i, y1, y2;
	float pixel;

	if (buflen == 0)
		return;

	for (i = 0; i <= buflen - 2; i++)
	{
		y1 = 1 - round(buf[i]);
	
		if (y1 > Image1Height - top_margin - bottom_margin - 2)
			y1 = Image1Height - top_margin - bottom_margin - 2;

		y2 = 1 - round(buf[i + 1]);

		if (y2 > Image1Height - top_margin - bottom_margin - 2)
			y2 = Image1Height - top_margin - bottom_margin - 2;

		// 150 pixels for 1000 Hz

		// i is the bin number, but bin is not 10 Hz but 12000 /1024
		// so freq = i * 12000 / 1024;
		// and pixel is freq * 300 /1000

		pixel = i * 12000.0f / 1024.0f;
		pixel = pixel * 150.0f /1000.0f;

		Painter->drawLine(
			left_margin + pixel,
			top_margin + y1,
			left_margin + pixel + 1,
			top_margin + y2);
	}
}

void make_graph_buf(float * buf, short tap, QPainter * Painter)
{
	int fft_size;
	float max;
	int i, k;

	fft_size = 1024; // 12000 / 10; // 10hz on sample;

	for (i = 0; i < tap; i++)
		prev_graph_buf[i]= 0;
	
	for (i = 0; i < fft_size; i++)
		src_graph_buf[i] = 0;

	src_graph_buf[0]= 1;

	FIR_filter(src_graph_buf, fft_size, tap, buf, graph_buf, prev_graph_buf);


	for (k = 0; k < fft_size; k++)
		RealIn[k] = graph_buf[k] * 32768;
	
	FourierTransform(fft_size, RealIn, RealOut, ImagOut, 0);

	for (k = 0; k < (fft_size / 2) - 1; k++)
		pnt_graph_buf[k] = powf(RealOut[k], 2) + powf(ImagOut[k], 2);

	max = 0;

	for (i = 0; i < (fft_size / 2) - 1; i++)
	{
		if (pnt_graph_buf[i] > max)
			max = pnt_graph_buf[i];
	}

	if (max > 0)
	{
		for (i = 0; i < (fft_size / 2) - 1; i++)
			pnt_graph_buf[i] = pnt_graph_buf[i] / max;
	}

	for (i = 0; i < (fft_size / 2) - 1; i++)
	{
		if (pnt_graph_buf[i] > 0)
			pnt_graph_buf[i] = 70 * log10(pnt_graph_buf[i]);

		else

			pnt_graph_buf[i] = 0;
	}

	filter_grid(Painter);

	Painter->setPen(Qt::blue);

	make_graph(pnt_graph_buf, 400, Painter);
}
