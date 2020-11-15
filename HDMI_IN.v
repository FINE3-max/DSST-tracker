`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2020/10/29 20:57:54
// Design Name: 
// Module Name: HDMI_IN
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


module HDMI_IN(
    input	wire 			iclk					,
	input	wire			s_rst_n					,
	
	input	wire			rx_over					,
	input	wire			cfg_done				,
	
	input 	wire 			hdmi_pclk				,
	input	wire	[23:0]	hdmi_data				,
	input 	wire 			hdmi_hs					,
	input 	wire 			hdmi_vs					,
	input 	wire 			hdmi_de					,
	
	
	output	wire 			hdmi_in_pclk			,
	output	wire	[23:0]	hdmi_in_data			,
	output	wire 			hdmi_in_hs				,
	output	wire 			hdmi_in_vs				,
	output	wire 			hdmi_in_de				,

	output	wire 			hdmi_en					,	//视频播放开始标志
	output	wire 			hdmi_vs_sel					//跳帧控制
    );
/**********************************************************************************************/
    reg				rx_over_r				='d0		;
    	//rx_over_r,接收完毕取全高
	always @(posedge iclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			rx_over_r 	<= 'd0;	
		end
		else begin
			if(rx_over) begin
				rx_over_r <= 'd1;
			end
		end
	end
/**********************************************************************************************/	
	reg		[23:0]	hdmi_in_data_r			='d0		;
	reg 			hdmi_in_hs_r			='d0		;
	reg 			hdmi_in_vs_r			='d0		;
	reg 			hdmi_in_de_r			='d0		;
		//接收，配置完成后，HDMI数据接入
	always @(posedge hdmi_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin	
			hdmi_in_data_r	<= 'd0;
			hdmi_in_hs_r	<= 'd0;
			hdmi_in_vs_r	<= 'd0;
			hdmi_in_de_r	<= 'd0;
		end
		else begin
			if(rx_over_r&&cfg_done) begin	
				hdmi_in_data_r	<=  hdmi_data	;
				hdmi_in_hs_r	<=  ~hdmi_hs	;
				hdmi_in_vs_r	<=  ~hdmi_vs	;
				hdmi_in_de_r	<=  hdmi_de		;
			end
			else begin
			    hdmi_in_data_r	<= 'd0;
			    hdmi_in_hs_r	<= 'd0;
			    hdmi_in_vs_r	<= 'd0;
			    hdmi_in_de_r	<= 'd0;
			end
		end
	end
	
	assign	hdmi_in_pclk	=	hdmi_pclk		;
	assign  hdmi_in_data    =	hdmi_in_data_r	;
	assign  hdmi_in_hs	    =	hdmi_in_hs_r	;
	assign  hdmi_in_vs	    =	hdmi_in_vs_r	;
	assign  hdmi_in_de	    =	hdmi_in_de_r	;
/**********************************************************************************************/
	reg		[23:0]	first_pix_r 			= 'd0		;
	reg				first_pix_en_r			= 'd0		;
	//每帧首个像素值
	always @(posedge hdmi_pclk or negedge s_rst_n)begin
		if(!s_rst_n || hdmi_in_vs_r) begin
			first_pix_r		<= 0 ;
			first_pix_en_r	<= 0 ;
		end
		else begin
			if(hdmi_in_de && (first_pix_en_r == 'd0)) begin
				first_pix_r 		<= hdmi_in_data;
				first_pix_en_r		<= 'd1 ;
			end	
		end
	end
/**********************************************************************************************/
	 reg             hdmi_mid_flag           ='d0;
    always @(posedge hdmi_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
		    hdmi_mid_flag <= 'd0;
		end
		else if(first_pix_r[23:20] >= 'hE)begin//检测到视频首帧为白色
		         hdmi_mid_flag <= 'd1;
//		    else begin
//		         hdmi_mid_flag <= 'd0;  
//		    end
		end
	end
/**********************************************************************************************/
	reg				hdmi_en_r				= 'd0		;
		//hdmi_en，视频数据输入有效信号，用来检测首帧
	always @(posedge hdmi_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			hdmi_en_r <= 'd0;
		end
		else begin
		      if(hdmi_en_r == 'd0)begin
		          if(hdmi_de && hdmi_vs) begin
                      if((hdmi_mid_flag == 'd1)&&(first_pix_r[23:20] < 'hE)&&(first_pix_r !== 'h00)) begin 
		                  hdmi_en_r <= 'd1;
		              end
		              else begin
		                  hdmi_en_r <= 'd0;//视频播放未开始
		              end
		          end
		      end
		end
	end
	
	assign hdmi_en = hdmi_en_r;
/**********************************************************************************************/	
	reg				hdmi_in_vs_posedge_r	= 'd0		;
	reg				hdmi_in_vs_posedge_r1	= 'd0		;
	reg				hdmi_in_vs_posedge_r2	= 'd0		;
	//hdmi_in_vs_posedge,捕捉场同步信号的上升沿
	always @(posedge hdmi_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			hdmi_in_vs_posedge_r1 <= 'd0;
			hdmi_in_vs_posedge_r2 <= 'd0;
		end
		else begin
			hdmi_in_vs_posedge_r1 <= hdmi_in_vs_r;
			hdmi_in_vs_posedge_r2 <= hdmi_in_vs_posedge_r1;
		end
	end

	always @(posedge hdmi_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			hdmi_in_vs_posedge_r <= 'd0;
		end
		else begin
			if((hdmi_in_vs_posedge_r1=='d1) &&(hdmi_in_vs_posedge_r2=='d0)) begin
				hdmi_in_vs_posedge_r <= 'd1;
			end
			else begin
				hdmi_in_vs_posedge_r <= 'd0;
			end
		end
	end
	
	reg		hdmi_vs_sel_r	= 'd1;
		//跳帧控制 hdmi_vs_sel ,为1当前帧有效，为0当前帧跳过
	always @(posedge hdmi_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			hdmi_vs_sel_r <= 'd1;
		end
		else begin
			if(hdmi_en_r == 'd1) begin
				if(hdmi_in_vs_posedge_r)  begin //vs上升沿到来时取反
					hdmi_vs_sel_r <= ~hdmi_vs_sel_r ;
                end
			end
		end
	end

	assign hdmi_vs_sel = hdmi_vs_sel_r;

endmodule
