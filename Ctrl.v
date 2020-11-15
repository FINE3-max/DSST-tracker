`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2020/10/11 19:52:00
// Design Name: 
// Module Name: Ctrl
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module Ctrl(
    input wire iclk,
    input wire s_rst_n,
    //UART
    input wire rx_over,
	input wire	[15:0]	initialpos_x,	
	input wire	[15:0]	initialpos_y,
	input wire	[15:0]	initialtarget_w,
	input wire	[15:0]	initialtarget_h,
	
	output wire tx_start,
	output wire [15:0] result_x,
	output wire [15:0] result_y,
	output wire [15:0] result_w,
	output wire [15:0] result_h,

	//HDMI_IN
	input wire hdmi_in_pclk,
	input wire hdmi_vs_sel,	//��֡����
	input wire hdmi_en,	//��Ƶ���ſ�ʼ�ź�
	
	//��֡��־
    output	wire	  first_frame_flag,	
	//IMG_CroppingRange
	input wire [11:0] pixel_x_start,
	input wire [11:0] pixel_y_start,	
	
	output wire IMG_CroppingRange_start,
	output wire [15:0] mid_pos_x,//�����720pԭͼ�У�Ŀ�����������е��м�λ��
	output wire [15:0] mid_pos_y,
	
	output wire [15:0] hls_pos_w,
	output wire [15:0] hls_pos_h,

    //DSST
    input wire			agg_result_x_ap_vld,
	input wire	[15:0]	agg_result_x,//IP�˼�������������������Ľ������
	input wire			agg_result_y_ap_vld,
	input wire	[15:0]	agg_result_y,
	
	input wire          agg_result_width_ap_vld,
	input wire	[15:0]	agg_result_width,
	input wire          agg_result_height_ap_vld,
	input wire	[15:0]  agg_result_height,
	
	input wire          agg_result_csf_ap_vld,
	input wire  [31:0]  agg_result_csf,
	output wire [31:0]  res_in_csf,
	
	input wire ap_done,
	output wire	ap_rst,
	output wire ap_start,
	
	//IMG_Cropping_RGB2GRAY
	input wire	IMG_Cropping_RGB2GRAY_over	

    );
/************************���������ԭͼ�� �����ĵ�����***************************************************************/
reg  [15:0] mid_pos_x_r;
reg  [15:0] mid_pos_y_r;

always@(posedge iclk or negedge s_rst_n)begin
    if(!s_rst_n)begin
        mid_pos_x_r <= 'd0;
        mid_pos_y_r <= 'd0;
    end
    else if(rx_over)begin//���ڵ�һ֡�Ĵ���
			mid_pos_x_r	<=   initialpos_x + initialtarget_w[15:1]; //����1λ�൱�ڳ���2
			mid_pos_y_r	<=   initialpos_y + initialtarget_h[15:1];    
    end
    else if(ap_done)begin
  			mid_pos_x_r	<=   agg_result_x[15:0] + pixel_x_start; 
			mid_pos_y_r	<=   agg_result_y[15:0] + pixel_y_start + 'd1;          
    end
end

assign	mid_pos_x = mid_pos_x_r;
assign	mid_pos_y = mid_pos_y_r;
/**************************************************************************************************************/
reg [15:0]  hls_pos_w_r;
reg [15:0]  hls_pos_h_r;
always@(posedge iclk or negedge s_rst_n)begin
    if(!s_rst_n)begin
        hls_pos_w_r <= 'd0;
        hls_pos_h_r <= 'd0;
    end
    else if(rx_over)begin
        hls_pos_w_r <= initialtarget_w;
        hls_pos_h_r <= initialtarget_h;
    end
    else if(ap_done)begin
        hls_pos_w_r <= agg_result_width;
        hls_pos_h_r <= agg_result_height;
    end
end

assign hls_pos_w = hls_pos_w_r;
assign hls_pos_h = hls_pos_h_r;
/**************************************************************************************************************/
reg [31:0] res_in_csf_r = 'd0;
always@(posedge iclk or negedge s_rst_n)begin
    if(!s_rst_n)begin
        res_in_csf_r <= 'd0;
    end
    else begin
        if(ap_start)begin
            res_in_csf_r <= agg_result_csf;
        end
    end
end

assign res_in_csf = res_in_csf_r;
/***********************************************************�ü�������Ŀ���********************************************************/
reg IMG_CroppingRange_start_r = 'd0;
reg IMG_CroppingRange_start_r1 = 'd0;
reg IMG_CroppingRange_start_r2 = 'd0;

always @(posedge iclk or negedge s_rst_n)begin
if(!s_rst_n) begin
	IMG_CroppingRange_start_r1	<= 'd0;
end
else begin
	if(rx_over || ap_done) begin//�ж�����
			IMG_CroppingRange_start_r1	<= 'd1;
	end
	else begin
			IMG_CroppingRange_start_r1	<= 'd0;
	end
end
end
//IMG_CroppingRange_start���ӳ�ap_done�������ڣ�ȷ��ʱ��ȫ
always @(posedge iclk or negedge s_rst_n)begin
if(!s_rst_n) begin
	IMG_CroppingRange_start_r2		<= 'd0;
	IMG_CroppingRange_start_r		<= 'd0;
end
else begin
	IMG_CroppingRange_start_r2	<= IMG_CroppingRange_start_r1;
	IMG_CroppingRange_start_r	<= IMG_CroppingRange_start_r2;
	end
end

assign	IMG_CroppingRange_start = IMG_CroppingRange_start_r	;

/**********************************��ģ��ͼ��д����źţ�д��ɺ󣬱���5��pclkʱ��*****************************************/
	
	reg			IMG_Cropping_RGB2GRAY_over_r	= 'd0;
	reg	[2:0]	cnt_IMG_Cropping_RGB2GRAY_over_r= 'd0;
	
	//cnt_IMG_Cropping_RGB2GRAY_over_r ����5��pclkʱ��
	always @(posedge hdmi_in_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			cnt_IMG_Cropping_RGB2GRAY_over_r	<= 'd0;
		end
		else begin
			if(IMG_Cropping_RGB2GRAY_over_r && cnt_IMG_Cropping_RGB2GRAY_over_r <= 'd4) begin
				cnt_IMG_Cropping_RGB2GRAY_over_r	<= cnt_IMG_Cropping_RGB2GRAY_over_r + 'd1;
			end
			else if(cnt_IMG_Cropping_RGB2GRAY_over_r == 'd5) begin
				cnt_IMG_Cropping_RGB2GRAY_over_r <= 'd0;
			end
			else begin
				cnt_IMG_Cropping_RGB2GRAY_over_r <= 'd0;
			end
		end
	end
	
	always @(posedge hdmi_in_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			IMG_Cropping_RGB2GRAY_over_r	<= 'd0;
		end
		else begin
			if(IMG_Cropping_RGB2GRAY_over) begin
				IMG_Cropping_RGB2GRAY_over_r	<= 'd1;
			end
			else if(cnt_IMG_Cropping_RGB2GRAY_over_r == 'd5) begin
				IMG_Cropping_RGB2GRAY_over_r	<= 'd0 ;
			end
		end
	end
/********************************************************DSST*************************************************************/
	//IMG_Cropping_RGB2GRAY_overΪ1ʱ����ʼ����
	//���������λ���ȴ��´ο�ʼ�����ź�
		
	reg				ap_rst_r		= 'd0	;
    reg				ap_start_r	    = 'd0   ;
	
	always @(posedge iclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			ap_rst_r	<= 'd1;	
			ap_start_r	<= 'd0;
		end
		else begin
			if(ap_done) begin
				ap_rst_r	<= 'd1;	
				ap_start_r	<= 'd0;
			end
			else if(IMG_Cropping_RGB2GRAY_over_r&&hdmi_en&&hdmi_vs_sel) begin
				ap_rst_r	<= 'd0;                    
			    ap_start_r	<= 'd1;
			end	
		end
	end
	
	assign	ap_start	= ap_start_r	;

/****************************************************���ս���ļ���*******************************************************/
reg	[15:0]	result_x_r	= 'd0;//�����ԭͼ�����Ͻ�����
reg	[15:0]	result_y_r	= 'd0;

always @(posedge iclk or negedge s_rst_n)begin
	if(!s_rst_n) begin
		result_x_r		<= 'd0;
		result_y_r		<= 'd0;
	end
	else begin
	   if(ap_done) begin
		  result_x_r	<= agg_result_x[15:0] + pixel_x_start - initialtarget_w[15:1];
		  result_y_r	<= agg_result_y[15:0] + pixel_y_start + 'd1 - initialtarget_h[15:1];
	   end	
	end
end

reg	[15:0]	result_w_r	= 'd0;
reg	[15:0]	result_h_r	= 'd0;

always @(posedge iclk or negedge s_rst_n)begin
	if(!s_rst_n) begin
	   result_w_r <= 'd0;
	   result_h_r <= 'd0;
	end
	else begin
	   if(ap_done)begin
	         result_w_r <= agg_result_width;
	         result_h_r <= agg_result_height;	       
	   end
	end
end

assign result_x = result_x_r;
assign result_y = result_y_r;
assign result_w = result_w_r;
assign result_h = result_h_r;
/*******************��ʼΪ1����һ֡����������Ϊ0��s_rst_n��λ���Ϊ1*************************/
	reg		first_frame_flag_r	= 'd1;

	//��֡��־����
	always @(posedge iclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			first_frame_flag_r <= 'd1;
		end
		else begin
			if(ap_done) begin
				first_frame_flag_r <= 'd0;
			end
		end
	end
	
	assign first_frame_flag	=	first_frame_flag_r;
/**************************************���Ϳ�ʼ�ź�****************************************************/
	//��ap_doneΪ���Ϳ�ʼ�ź�������
	//tx_start�ӳ�ap_done����ʱ�����ڷ��ͣ�ȷ��ʱ��ȫ
	reg			tx_start_r			= 'd0;	//ap_doneΪ1������һ�����ڣ�����ʱ��Ϊ0
	reg			tx_start_r1			= 'd0;
	reg			tx_start_r2			= 'd0;
	always @(posedge iclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			tx_start_r1		<= 'd0;
		end
		else begin
			if(ap_done&&(!first_frame_flag_r)) begin		//��֡��Ϣ������
				tx_start_r1	<= 'd1;
			end
			else begin
				tx_start_r1	<= 'd0;
			end
		end
	end
	
	always @(posedge iclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			tx_start_r2		<= 'd0;
			tx_start_r		<= 'd0;
		end
		else begin
			tx_start_r2	<= tx_start_r1;
			tx_start_r	<= tx_start_r2;
		end
	end
	
	assign tx_start	=	tx_start_r;

	//ʹ��BUFG����ap_rst���ȳ�
	BUFG AP_RST(
        .I	(ap_rst_r	),
        .O	(ap_rst		)
    );
endmodule
