`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2020/10/11 21:59:02
// Design Name: 
// Module Name: IMG_RGB2GRAY
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


module IMG_RGB2GRAY(
        input	wire			s_rst_n						,	
		
		output	wire			IMG_Cropping_RGB2GRAY_over	,//��ģ��ͼ��д����źţ�ʱ��ΪHDMI����ʱ��
		
		//HDMI
		input 	wire 			hdmi_in_pclk				,		
		input	wire	[23:0]	hdmi_in_data				,
		input 	wire 			hdmi_in_hs					,
        input 	wire 			hdmi_in_vs					,
		input 	wire 			hdmi_in_de					,
		
		//����RAM���귶Χ		
		input	wire	[11:0]	pixel_x_start 				,
		input	wire	[11:0]	pixel_x_end 				,
		input	wire	[11:0]	pixel_y_start 				,
		input	wire	[11:0]	pixel_y_end 				,

		output	reg		[ 7:0]	gray_data					,
		output 	reg 	[15:0]  gray_data_addr				,
		output 	reg 			gray_data_we			
    );
    parameter      PIXEL_TOTAL                     = 44999   ;//15*300=45000-1
	reg				IMG_Cropping_RGB2GRAY_over_r	= 'd0		;
	
	reg		[11:0]	cnt_pixel	        			= 'd0		;
	reg		[11:0]	cnt_line	        			= 'd0		;
	reg		[ 7:0]	gray_data_a1        			= 'd0		;
	reg 	[15:0]  gray_data_addr_a1				= 'd0		;
	
//IMG_Cropping_RGB2GRAY_over_r,ԭʼ����д�����imgRAM��ɣ�����������һ������
	always @(posedge hdmi_in_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			IMG_Cropping_RGB2GRAY_over_r <= 'd0;
		end
		else begin
			if(gray_data_addr == PIXEL_TOTAL + 'd1) begin
				IMG_Cropping_RGB2GRAY_over_r <= 'd1;
			end
			else begin
				IMG_Cropping_RGB2GRAY_over_r <= 'd0;
			end
		end
	end
	
	assign IMG_Cropping_RGB2GRAY_over = IMG_Cropping_RGB2GRAY_over_r;

//cnt_pixel	1-1280 �м���
	always @(posedge hdmi_in_pclk or negedge s_rst_n)begin
		if((!s_rst_n) || (hdmi_in_vs)) begin
			cnt_pixel <= 'd0;
		end
		else begin
			if(hdmi_in_de == 'd1) begin
				if(cnt_pixel < 'd1280) begin
					cnt_pixel <= cnt_pixel + 1;
				end
				else if(cnt_pixel == 'd1280)begin
					cnt_pixel <= 'd1;
				end
			end
			else begin
				cnt_pixel <= cnt_pixel;
			end
		end
	end
	
//cnt_line 0-719 �м���
	always @(posedge hdmi_in_pclk or negedge s_rst_n)begin
		if((!s_rst_n) || (hdmi_in_vs)) begin
			cnt_line <= 0;
		end
		else begin
			if((cnt_pixel == 'd1280)&&hdmi_in_de) begin
				cnt_line <= cnt_line + 1;
			end
		end
	end
	
//gray_data_a1���Ҷ�ֵ���㣬����ʵ��ǰһ��ʱ������
	always @(posedge hdmi_in_pclk or negedge s_rst_n)begin
		if(!s_rst_n) begin
			gray_data_a1 <= 'd0;
		end
		else if(hdmi_in_de)begin
			gray_data_a1 <= ((hdmi_in_data[23:16]*38 + hdmi_in_data[15:8]*75 +hdmi_in_data[7:0]*15) >> 7);
		end
	end
	
//gray_data_addr_a1������ʵ����ǰһ��ʱ�����ڣ�gray_data_we
	always @(posedge hdmi_in_pclk or negedge s_rst_n)begin
		if((!s_rst_n) || (hdmi_in_vs)) begin
			gray_data_addr_a1 	<= 'd0;
			gray_data_we 		<= 'd0;
		end
		else begin
			if((cnt_pixel >= pixel_x_start)&&(cnt_pixel <= pixel_x_end)&&(cnt_line >= pixel_y_start)&&(cnt_line <= pixel_y_end)) begin
				if(hdmi_in_de == 'd1) begin
					gray_data_addr_a1 <= gray_data_addr_a1 + 'd1 ;
					gray_data_we <= 'd1;
				end
				else begin
					gray_data_we <= 'd0;
				end
			end
			else begin
				if(gray_data_addr_a1 == (PIXEL_TOTAL + 'd1)) begin
					gray_data_addr_a1 <= 'd0;
				end
				gray_data_we <= 'd0;
			end
		end
	end

//ÿһ֡ͬ��ʱ��gray_data_addrˢ��Ϊ0
	always @(posedge hdmi_in_pclk or negedge s_rst_n)begin
		if((!s_rst_n) || (hdmi_in_vs)) begin
			gray_data_addr <= 0;
		end
		else begin
			gray_data_addr <= gray_data_addr_a1;
		end
	end	

//gray_data	��д��ram�ĻҶ�ֵ
	 always @(posedge hdmi_in_pclk or negedge s_rst_n)begin
		 if(!s_rst_n) begin
			 gray_data <= 0;
		 end
		 else begin
			 gray_data <= gray_data_a1;
		 end
	 end

endmodule
