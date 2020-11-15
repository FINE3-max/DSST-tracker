`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2020/10/11 16:53:12
// Design Name: 
// Module Name: TOP
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


module TOP(
		input	wire			s_rst_n					,
	
		input  	wire			iclk_p					,
        input  	wire			iclk_n					,
	
		//HDMI
		output 	wire			adv_rst					,
		output 	wire			pen_o					,
		
		inout  	wire			adv_sda					,
		output 	wire			adv_scl					,
		
		input 	wire 			hdmi_pclk				,	
		input	wire	[23:0]	hdmi_data				,
		input 	wire 			hdmi_hs					,//低有效
        input 	wire 			hdmi_vs					,//低有效
		input 	wire 			hdmi_de					,
		
		//UART
		input 	wire			uart_rx_i				,
		output  wire			uart_tx_o				,
		
		output  wire            HDMI_CLK_P              ,
        output  wire            HDMI_CLK_N              ,
        output  wire    [2:0]   HDMI_TX_P               ,
        output  wire    [2:0]   HDMI_TX_N
    );
/*************************************************************************/
wire CLK_IN,clk_100;
IBUFGDS CLK_IN_IBUFGDS_inst(
   .I(iclk_p),
   .IB(iclk_n),
   .O(CLK_IN));
BUFG CLK_BUFG_inst(
    .I(CLK_IN),
    .O(clk_100));
    
wire pclkx1,pclkx5;
wire locked;
clk_wiz_0 clk_inst
   (
    // Clock out ports
    .clk_out1(pclkx1),     // output clk_out1
    .clk_out2(pclkx5),     // output clk_out2
    // Status and control signals
    .locked(locked),       // output locked
   // Clock in ports
    .clk_in1(hdmi_pclk));      // input clk_in1
/*************************************************************************/    
wire				rx_over				;
wire	[15:0]		initialpos_x    	;
wire	[15:0]		initialpos_y    	;
wire	[15:0]		initialtarget_w	    ;
wire	[15:0]		initialtarget_h     ;  
wire				tx_start			;
wire	[15:0]		result_x			;
wire	[15:0]		result_y			;
wire	[15:0]		result_w			;
wire	[15:0]		result_h			;

uart_top uart_top_inst( 
    .iclk		    (clk_100     		),
    .s_rst_n		(s_rst_n			),
    
    .uart_rx_i      (uart_rx_i		 	),//rx_input
    .rx_over		(rx_over			),//rx_output
    .initialpos_x   (initialpos_x		),
    .initialpos_y   (initialpos_y		),
    .initialtarget_w(initialtarget_w	),
    .initialtarget_h(initialtarget_h    ),

	.tx_start       (tx_start		 	),//tx_input
	.result_x       (result_x		 	),
	.result_y       (result_y			),
	.result_w       (result_w		 	),
	.result_h       (result_h			),
    .uart_tx_o      (uart_tx_o		 	)//tx_output
);

/*************************************************************************/
wire cfg_done,rst_o;
uicfg7611_0 uicfg7611_0_inst (
  .clk_i(clk_100),        // input wire clk_i
  .rst_n(rst_o),        // input wire rst_n
  .adv_scl(adv_scl),    // output wire adv_scl
  .adv_sda(adv_sda),    // inout wire adv_sda
  .cfg_done(cfg_done)  // output wire cfg_done
);
assign pen_o = 1'b1; 
assign adv_rst = 1'b1; 
/*************************************************************************/
	wire 				hdmi_in_pclk				;
	wire	[23:0]		hdmi_in_data				;
	wire 				hdmi_in_hs					;
	wire 				hdmi_in_vs					;
	wire 				hdmi_in_de					;
	wire 				hdmi_en						;
	wire 				hdmi_vs_sel					;
	
HDMI_IN HDMI_IN_inst( 
        .iclk		     	(clk_100		    	),
        .s_rst_n	        (s_rst_n				),
	
		.rx_over			(rx_over				),//input
		.cfg_done			(cfg_done				),
		
		.hdmi_pclk			(hdmi_pclk				),//input
		.hdmi_data			(hdmi_data				),
		.hdmi_hs			(hdmi_hs				),
		.hdmi_vs			(hdmi_vs				),
		.hdmi_de			(hdmi_de				),
		
		.hdmi_in_pclk		(hdmi_in_pclk			),//output
		.hdmi_in_data		(hdmi_in_data			),
		.hdmi_in_hs			(hdmi_in_hs				),
		.hdmi_in_vs			(hdmi_in_vs				),
		.hdmi_in_de			(hdmi_in_de				),
		
		.hdmi_en			(hdmi_en				),
		.hdmi_vs_sel		(hdmi_vs_sel			)//output
	);
/*************************************************************************/
	wire				IMG_CroppingRange_start		;
	
	wire	[15:0]		mid_pos_x					;
	wire	[15:0]		mid_pos_y					;

	wire	[15:0]		hls_pos_x					;
	wire	[15:0]		hls_pos_y               	;
	
	wire	[11:0]		pixel_x_start 				;
	wire	[11:0]		pixel_x_end 				;
	wire	[11:0]		pixel_y_start 				;
	wire	[11:0]		pixel_y_end 				;	
	
Img_CroppingRange Img_CroppingRange_inst(
        .rst_n				    (s_rst_n					)	,
		.clk					(clk_100			 		)	,
	
		.IMG_CroppingRange_start(IMG_CroppingRange_start	)	,//input
		
		.mid_pos_x	        	(mid_pos_x					)	,//input
		.mid_pos_y				(mid_pos_y					)	,
	
		.hls_pos_x				(hls_pos_x					)	,//output-149
		.hls_pos_y				(hls_pos_y					)	,//output-74
		
		.pixel_x_start 			(pixel_x_start 				)	,//output
		.pixel_x_end 			(pixel_x_end 				)	,
		.pixel_y_start 			(pixel_y_start 				)	,
		.pixel_y_end 			(pixel_y_end 				)	
    );
 /*************************************************************************/
    wire				IMG_Cropping_RGB2GRAY_over		;
	wire	[ 7:0]		gray_data						;
	wire	[15:0]		gray_data_addr					;
	wire				gray_data_we					;
	
IMG_RGB2GRAY IMG_RGB2GRAY_inst (
		.s_rst_n						(s_rst_n					)	,
		
		.IMG_Cropping_RGB2GRAY_over		(IMG_Cropping_RGB2GRAY_over	)	,
		
		.hdmi_in_pclk					(hdmi_in_pclk			 	)	,
	    .hdmi_in_data					(hdmi_in_data				)	,
	    .hdmi_in_hs						(hdmi_in_hs					)	,
	    .hdmi_in_vs						(hdmi_in_vs					)	,
	    .hdmi_in_de						(hdmi_in_de					)	,
		
		.pixel_x_start 					(pixel_x_start 				)	,
		.pixel_x_end 					(pixel_x_end 				)	,
		.pixel_y_start 					(pixel_y_start 				)	,
		.pixel_y_end 					(pixel_y_end 				)	,
		
		.gray_data						(gray_data					)	,
		.gray_data_addr					(gray_data_addr				)	,
		.gray_data_we					(gray_data_we				)	
	);
 /*************************************************************************/
    wire               first_frame_flag             ;
 	wire				ap_rst						;
	wire            	ap_start	                ;
	wire            	ap_done	                    ;
	wire				ap_idle                     ;
	wire				ap_ready                    ;
	
	//结果输出                                      
	wire	[15:0]		agg_result_x		        ;                          
	wire				agg_result_x_ap_vld         ;                         
	wire	[15:0]		agg_result_y                ;                         
	wire				agg_result_y_ap_vld         ;     
	 
	wire	[15:0]		agg_result_width		    ;                         
	wire				agg_result_width_ap_vld     ;                         
	wire	[15:0]		agg_result_height           ;                         
	wire				agg_result_height_ap_vld    ; 
	wire    [31:0]     res_in_csf                   ;    
    wire               agg_result_csf_ap_vld        ;
    wire    [31:0]     agg_result_csf               ;              
	
	wire    [15:0]     hls_pos_w                    ;
	wire    [15:0]     hls_pos_h                    ;
	
 Ctrl Ctrl_inst(
        .iclk							(clk_100						),
		.s_rst_n						(s_rst_n						),
		//UART
		.rx_over						(rx_over						),//input
		.initialpos_x					(initialpos_x					),		
        .initialpos_y					(initialpos_y					), 
		.initialtarget_w				(initialtarget_w				),
		.initialtarget_h				(initialtarget_h				),		
		
		.tx_start						(tx_start						), //output
		.result_x						(result_x						),
		.result_y						(result_y						),		
		.result_w                       (result_w                       ),
		.result_h                       (result_h                       ),
		
		//HDMI_ctrl
		.hdmi_in_pclk					(hdmi_in_pclk			 		),
		.hdmi_en						(hdmi_en			 			),
		.hdmi_vs_sel					(hdmi_vs_sel					),
		
		//首帧标志
		.first_frame_flag				(first_frame_flag				),//output
		
	    //IMG_CroppingRange
	    .pixel_x_start 					(pixel_x_start 					),//input
		.pixel_y_start 					(pixel_y_start 					),
		
		.IMG_CroppingRange_start		(IMG_CroppingRange_start		),//output
		.mid_pos_x						(mid_pos_x 						),
		.mid_pos_y						(mid_pos_y 						),
		
		.hls_pos_w                      (hls_pos_w                      ),//output
		.hls_pos_h                      (hls_pos_h                      ),

		//DSST
		.ap_rst							(ap_rst							),//output
		.ap_start						(ap_start						),
		.ap_done						(ap_done						),//input	
		
        .agg_result_x_ap_vld  			(agg_result_x_ap_vld  			),//input
		.agg_result_x		 			(agg_result_x		 			), 
		.agg_result_y_ap_vld  			(agg_result_y_ap_vld   			),
		.agg_result_y         			(agg_result_y          			),
		.agg_result_width_ap_vld        (agg_result_width_ap_vld        ),
		.agg_result_width               (agg_result_width               ),
		.agg_result_height_ap_vld       (agg_result_height_ap_vld       ),
		.agg_result_height              (agg_result_height              ),
		.agg_result_csf_ap_vld          (agg_result_csf_ap_vld          ),
        .agg_result_csf                 (agg_result_csf                 ),       
        .res_in_csf                     (res_in_csf                     ),           
		
		//IMG_Cropping_RGB2GRAY
		.IMG_Cropping_RGB2GRAY_over		(IMG_Cropping_RGB2GRAY_over		)//input
 );
  /*************************************************************************/
  	//img原始图像输入RAM                            
	wire				img_ce0	                    ;                
	wire				img_we0	                    ;                
	wire	[15:0]		img_address0                ;                    
	wire	[ 9:0]		img_q0		                ;                     
	wire	[ 9:0]		img_d0		                ;     
	//model_xf_real RAM                             
	wire				model_xf_real_ce0		    ;                                      
	wire				model_xf_real_we0		    ;                                      
	wire	[14:0]		model_xf_real_address0	    ;                                      
	wire	[31:0]		model_xf_real_q0		    ;                                      
	wire	[31:0]		model_xf_real_d0		    ;                                      

	wire				model_xf_real_ce1		    ;                                      
	wire				model_xf_real_we1		    ;                                      
	wire	[14:0]		model_xf_real_address1	    ;                                      
	wire	[31:0]		model_xf_real_q1		    ;                                      
	wire	[31:0]		model_xf_real_d1		    ;                                      

	//model_xf_imag                                 
	wire				model_xf_imag_ce0		    ;                                      
	wire				model_xf_imag_we0		    ;               
	wire	[14:0]		model_xf_imag_address0	    ;               
	wire	[31:0]		model_xf_imag_q0		    ;               
	wire	[31:0]		model_xf_imag_d0		    ;               

	wire				model_xf_imag_ce1		    ;               
	wire				model_xf_imag_we1		    ;               
	wire	[14:0]		model_xf_imag_address1	    ;               
	wire	[31:0]		model_xf_imag_q1		    ;               
	wire	[31:0]		model_xf_imag_d1            ;

	//model_alphaf                                  
	wire				model_alphaf_ce0	        ;                            
	wire				model_alphaf_we0	        ;                            
	wire	[11:0]		model_alphaf_address0       ;                             
	wire	[31:0]		model_alphaf_q0		        ;                             
	wire	[31:0]		model_alphaf_d0	            ;
	
	wire				model_alphaf_ce1	        ;    
	                                                    
	wire	[11:0]		model_alphaf_address1       ;                             
	wire	[31:0]		model_alphaf_q1		        ;                             

	//yf                                            
	wire				yf_ce0	                    ;                                
	wire	[11:0]		yf_address0                 ;                   
	wire	[31:0]		yf_q0		                ;                     
	
	//cos_window                                    
	wire				cos_window_ce0	            ;                          
	wire	[11:0]		cos_window_address0         ;                           
	wire	[31:0]		cos_window_q0		        ;   
	
//current scale factor

wire scale_model_sz_ce0              ;
wire scale_model_sz_we0              ;
wire [1 : 0] scale_model_sz_address0 ;
wire [31 : 0] scale_model_sz_d0      ;
wire [31 : 0] scale_model_sz_q0      ;

wire         scale_model_sz_ce1     ;
wire         scale_model_sz_we1     ;
wire [1 : 0] scale_model_sz_address1;
wire [31 : 0] scale_model_sz_q1     ; 
wire [31 : 0] scale_model_sz_d1     ;

wire scale_factor_ce0             ;
wire scale_factor_we0             ;
wire [0 : 0] scale_factor_address0;
wire [31 : 0] scale_factor_d0     ;
wire [31 : 0] scale_factor_q0     ;

wire         scale_factor_ce1     ;
wire         scale_factor_we1     ;
wire         scale_factor_address1;
wire [31 : 0] scale_factor_q1     ;
wire [31 : 0] scale_factor_d1     ;

wire new_sf_num_real_ce0              ;
wire new_sf_num_real_we0              ;
wire [13 : 0] new_sf_num_real_address0;
wire [31 : 0] new_sf_num_real_q0      ;
wire [31 : 0] new_sf_num_real_d0      ;

wire new_sf_num_imag_ce0              ;
wire new_sf_num_imag_we0              ;
wire [13 : 0] new_sf_num_imag_address0;
wire [31 : 0] new_sf_num_imag_q0      ;
wire [31 : 0] new_sf_num_imag_d0      ;

wire new_sf_den_ce0             ;
wire new_sf_den_we0             ;
wire [4 : 0] new_sf_den_address0;
wire [31 : 0] new_sf_den_q0     ;
wire [31 : 0] new_sf_den_d0     ;

wire sf_num_real_ce0              ;
wire sf_num_real_we0              ;
wire [13 : 0] sf_num_real_address0;
wire [31 : 0] sf_num_real_q0      ; 
wire [31 : 0] sf_num_real_d0      ;

wire sf_num_imag_ce0              ;
wire sf_num_imag_we0              ;
wire [13 : 0] sf_num_imag_address0;
wire [31 : 0] sf_num_imag_d0      ;
wire [31 : 0] sf_num_imag_q0      ;

wire sf_den_ce0             ;
wire sf_den_we0             ;
wire [4 : 0] sf_den_address0;
wire [31 : 0] sf_den_q0     ;
wire [31 : 0] sf_den_d0     ;

wire         sf_num_real_ce1        ;
wire         sf_num_real_we1        ;
wire [13 : 0] sf_num_real_address1  ;
wire [31 : 0] sf_num_real_d1        ;

wire         sf_den_ce1     ;
wire         sf_den_we1     ;
wire [4 : 0] sf_den_address1;
wire [31 : 0] sf_den_d1     ;
KCF_tracker_1 DSST (

  .ap_clk(clk_100),                                      // input wire ap_clk
  .ap_rst(ap_rst),                                      // input wire ap_rst
  .ap_start(ap_start),                                  // input wire ap_start
  .ap_done(ap_done),                                    // output wire ap_done
  .ap_idle(ap_idle),                                    // output wire ap_idle
  .ap_ready(ap_ready),                                  // output wire ap_ready

  .frame(first_frame_flag),                             // input wire [0 : 0] frame
  
  .res_in_x(hls_pos_x),                                  // input wire [15 : 0] res_in_x
  .res_in_y(hls_pos_y),                                  // input wire [15 : 0] res_in_y
  .res_in_width(hls_pos_w),                          // input wire [15 : 0] res_in_width
  .res_in_height(hls_pos_h),                        // input wire [15 : 0] res_in_height
  
  .agg_result_x_ap_vld(agg_result_x_ap_vld),            // output wire agg_result_x_ap_vld
  .agg_result_y_ap_vld(agg_result_y_ap_vld),            // output wire agg_result_y_ap_vld
  .agg_result_width_ap_vld(agg_result_width_ap_vld),    // output wire agg_result_width_ap_vld
  .agg_result_height_ap_vld(agg_result_height_ap_vld),  // output wire agg_result_height_ap_vld
  
  .agg_result_x(agg_result_x),                          // output wire [15 : 0] agg_result_x
  .agg_result_y(agg_result_y),                          // output wire [15 : 0] agg_result_y
  .agg_result_width(agg_result_width),                  // output wire [15 : 0] agg_result_width
  .agg_result_height(agg_result_height),                // output wire [15 : 0] agg_result_height
  
  .res_in_csf(res_in_csf),                              // input wire [31 : 0] res_in_csf  
  .agg_result_csf_ap_vld(agg_result_csf_ap_vld),        // output wire agg_result_csf_ap_vld
  .agg_result_csf(agg_result_csf),                      // output wire [31 : 0] agg_result_csf  
  
  .img_ce0(img_ce0),                                    // output wire img_ce0
  .img_we0(img_we0),                                    // output wire img_we0
  .img_address0(img_address0),                          // output wire [15 : 0] img_address0
  .img_d0(img_d0),                                      // output wire [9 : 0] img_d0
  .img_q0(img_q0),                                      // input wire [9 : 0] img_q0  
  
  .model_xf_real_ce0(model_xf_real_ce0),                // output wire model_xf_real_ce0
  .model_xf_real_we0(model_xf_real_we0),                // output wire model_xf_real_we0
  .model_xf_real_address0(model_xf_real_address0),      // output wire [14 : 0] model_xf_real_address0
  .model_xf_real_d0(model_xf_real_d0),                  // output wire [31 : 0] model_xf_real_d0
  .model_xf_real_q0(model_xf_real_q0),                  // input wire [31 : 0] model_xf_real_q0  
  
  .model_xf_real_ce1(model_xf_real_ce1),                // output wire [0 : 0] model_xf_real_ce1
  .model_xf_real_we1(model_xf_real_we1),                // output wire [0 : 0] model_xf_real_we1
  .model_xf_real_address1(model_xf_real_address1),      // output wire [14 : 0] model_xf_real_address1
  .model_xf_real_d1(model_xf_real_d1),                  // output wire [31 : 0] model_xf_real_d1
  .model_xf_real_q1(model_xf_real_q1),                  // input wire [31 : 0] model_xf_real_q1 
  
  .model_xf_imag_ce0(model_xf_imag_ce0),                // output wire model_xf_imag_ce0
  .model_xf_imag_we0(model_xf_imag_we0),                // output wire model_xf_imag_we0
  .model_xf_imag_address0(model_xf_imag_address0),      // output wire [14 : 0] model_xf_imag_address0
  .model_xf_imag_d0(model_xf_imag_d0),                  // output wire [31 : 0] model_xf_imag_d0
  .model_xf_imag_q0(model_xf_imag_q0),                  // input wire [31 : 0] model_xf_imag_q0  
  
  .model_xf_imag_ce1(model_xf_imag_ce1),                // output wire [0 : 0] model_xf_imag_ce1
  .model_xf_imag_we1(model_xf_imag_we1),                // output wire [0 : 0] model_xf_imag_we1
  .model_xf_imag_address1(model_xf_imag_address1),      // output wire [14 : 0] model_xf_imag_address1
  .model_xf_imag_d1(model_xf_imag_d1),                  // output wire [31 : 0] model_xf_imag_d1
  .model_xf_imag_q1(model_xf_imag_q1),                  // input wire [31 : 0] model_xf_imag_q1

  .model_alphaf_ce0(model_alphaf_ce0),                  // output wire model_alphaf_ce0
  .model_alphaf_we0(model_alphaf_we0),                  // output wire model_alphaf_we0
  .model_alphaf_address0(model_alphaf_address0),        // output wire [11 : 0] model_alphaf_address0
  .model_alphaf_d0(model_alphaf_d0),                    // output wire [31 : 0] model_alphaf_d0
  .model_alphaf_q0(model_alphaf_q0),                    // input wire [31 : 0] model_alphaf_q0  
  
  .model_alphaf_ce1(model_alphaf_ce1),                  // output wire [0 : 0] model_alphaf_ce1
//
  .model_alphaf_address1(model_alphaf_address1),        // output wire [11 : 0] model_alphaf_address1
//
  .model_alphaf_q1(model_alphaf_q1),                    // input wire [31 : 0] model_alphaf_q1  
  
  .yf_ce0(yf_ce0),                                      // output wire yf_ce0
  .yf_address0(yf_address0),                            // output wire [11 : 0] yf_address0
  .yf_q0(yf_q0),                                        // input wire [31 : 0] yf_q0
  
  .cos_window_ce0(cos_window_ce0),                      // output wire cos_window_ce0
  .cos_window_address0(cos_window_address0),            // output wire [11 : 0] cos_window_address0
  .cos_window_q0(cos_window_q0),                        // input wire [31 : 0] cos_window_q0  
  
  .scale_model_sz_ce0(scale_model_sz_ce0),              // output wire scale_model_sz_ce0
  .scale_model_sz_we0(scale_model_sz_we0),              // output wire scale_model_sz_we0
  .scale_model_sz_address0(scale_model_sz_address0),    // output wire [1 : 0] scale_model_sz_address0
  .scale_model_sz_d0(scale_model_sz_d0),                // output wire [31 : 0] scale_model_sz_d0
  .scale_model_sz_q0(scale_model_sz_q0),                // input wire [31 : 0] scale_model_sz_q0  
  
  .scale_model_sz_ce1(scale_model_sz_ce1),              // output wire [0 : 0] scale_model_sz_ce1
  .scale_model_sz_we1(scale_model_sz_we1),              // output wire [0 : 0] scale_model_sz_we1  
  .scale_model_sz_address1(scale_model_sz_address1),    // output wire [1 : 0] scale_model_sz_address1
  .scale_model_sz_d1(scale_model_sz_d1),                // output wire [31 : 0] scale_model_sz_d1
  .scale_model_sz_q1(scale_model_sz_q1),                // input wire [31 : 0] scale_model_sz_q1  
  
  .scale_factor_ce0(scale_factor_ce0),                  // output wire scale_factor_ce0
  .scale_factor_we0(scale_factor_we0),                  // output wire scale_factor_we0
  .scale_factor_address0(scale_factor_address0),        // output wire [0 : 0] scale_factor_address0
  .scale_factor_d0(scale_factor_d0),                    // output wire [31 : 0] scale_factor_d0
  .scale_factor_q0(scale_factor_q0),                    // input wire [31 : 0] scale_factor_q0
  
  .scale_factor_ce1(scale_factor_ce1),                  // output wire [0 : 0] scale_factor_ce1
  .scale_factor_we1(scale_factor_we1),                  // output wire [0 : 0] scale_factor_we1
  .scale_factor_address1(scale_factor_address1),        // output wire [0 : 0] scale_factor_address1
  .scale_factor_d1(scale_factor_d1),                    // output wire [31 : 0] scale_factor_d1
  .scale_factor_q1(scale_factor_q1),                    // input wire [31 : 0] scale_factor_q1  
  
  .sf_num_real_ce0(sf_num_real_ce0),                    // output wire sf_num_real_ce0
  .sf_num_real_we0(sf_num_real_we0),                    // output wire sf_num_real_we0
  .sf_num_real_address0(sf_num_real_address0),          // output wire [13 : 0] sf_num_real_address0
  .sf_num_real_d0(sf_num_real_d0),                      // output wire [31 : 0] sf_num_real_d0
  .sf_num_real_q0(sf_num_real_q0),                      // input wire [31 : 0] sf_num_real_q0  
  
  .sf_num_imag_ce0(sf_num_imag_ce0),                    // output wire sf_num_imag_ce0
  .sf_num_imag_we0(sf_num_imag_we0),                    // output wire sf_num_imag_we0
  .sf_num_imag_address0(sf_num_imag_address0),          // output wire [13 : 0] sf_num_imag_address0
  .sf_num_imag_d0(sf_num_imag_d0),                      // output wire [31 : 0] sf_num_imag_d0
  .sf_num_imag_q0(sf_num_imag_q0),                      // input wire [31 : 0] sf_num_imag_q0  
  
  .sf_den_ce0(sf_den_ce0),                              // output wire sf_den_ce0
  .sf_den_we0(sf_den_we0),                              // output wire sf_den_we0
  .sf_den_address0(sf_den_address0),                    // output wire [4 : 0] sf_den_address0
  .sf_den_d0(sf_den_d0),                                // output wire [31 : 0] sf_den_d0
  .sf_den_q0(sf_den_q0)                                // input wire [31 : 0] sf_den_q0
);

 /***********************************BLOCK RAM**************************************/
img img_inst (
  .clka(hdmi_in_pclk),    // input wire clka
  .ena(hdmi_vs_sel),      // input wire ena
  .wea(gray_data_we),      // input wire [0 : 0] wea
  .addra(gray_data_addr),  // input wire [15 : 0] addra
  .dina({2'b0,gray_data[7:0]}),    // input wire [9 : 0] dina
  .douta(                    ),  // output wire [9 : 0] douta
  
  .clkb(clk_100),    // input wire clkb
  .enb(img_ce0),      // input wire enb
  .web(img_we0),      // input wire [0 : 0] web
  .addrb(img_address0),  // input wire [15 : 0] addrb
  .dinb(img_d0),    // input wire [9 : 0] dinb
  .doutb(img_q0)  // output wire [9 : 0] doutb
);

model_xf_real model_xf_real_inst (
  .clka(clk_100),    // input wire clka
  .ena(model_xf_real_ce0),      // input wire ena
  .wea(model_xf_real_we0),      // input wire [0 : 0] wea
  .addra(model_xf_real_address0),  // input wire [14 : 0] addra
  .dina(model_xf_real_d0),    // input wire [31 : 0] dina
  .douta(model_xf_real_q0),  // output wire [31 : 0] douta
  
  .clkb(clk_100),    // input wire clkb
  .enb(model_xf_real_ce1),      // input wire enb
  .web(model_xf_real_we1),      // input wire [0 : 0] web
  .addrb(model_xf_real_address1),  // input wire [14 : 0] addrb
  .dinb(model_xf_real_d1),    // input wire [31 : 0] dinb
  .doutb(model_xf_real_q1)  // output wire [31 : 0] doutb
);

model_xf_imag model_xf_imag_inst (
  .clka(clk_100),    // input wire clka
  .ena(model_xf_imag_ce0),      // input wire ena
  .wea(model_xf_imag_we0),      // input wire [0 : 0] wea
  .addra(model_xf_imag_address0),  // input wire [14 : 0] addra
  .dina(model_xf_imag_d0),    // input wire [31 : 0] dina
  .douta(model_xf_imag_q0),  // output wire [31 : 0] douta
  
  .clkb(clk_100),    // input wire clkb
  .enb(model_xf_imag_ce1),      // input wire enb
  .web(model_xf_imag_we1),      // input wire [0 : 0] web
  .addrb(model_xf_imag_address1),  // input wire [14 : 0] addrb
  .dinb(model_xf_imag_d1),    // input wire [31 : 0] dinb
  .doutb(model_xf_imag_q1)  // output wire [31 : 0] doutb
);

model_alphaf model_alphaf_inst (
  .clka(clk_100),    // input wire clka
  .ena(model_alphaf_ce0),      // input wire ena
  .wea(model_alphaf_we0),      // input wire [0 : 0] wea
  .addra(model_alphaf_address0),  // input wire [11 : 0] addra
  .dina(model_alphaf_d0),    // input wire [31 : 0] dina
  .douta(model_alphaf_q0),  // output wire [31 : 0] douta
  
  .clkb(clk_100),    // input wire clkb
  .enb(model_alphaf_ce1),      // input wire enb
  .web('d0),      // input wire [0 : 0] web
  .addrb(model_alphaf_address1),  // input wire [11 : 0] addrb
  .dinb('d0),    // input wire [31 : 0] dinb
  .doutb(model_alphaf_q1)  // output wire [31 : 0] doutb
);

yf yf_inst (
  .clka(clk_100),    // input wire clka
  .ena(yf_ce0),      // input wire ena
  .wea('d0),      // input wire [0 : 0] wea
  .addra(yf_address0),  // input wire [11 : 0] addra
  .dina('d0),    // input wire [31 : 0] dina
  .douta(yf_q0)  // output wire [31 : 0] douta
);

cos_window cos_window_inst (
  .clka(clk_100),    // input wire clka
  .ena(cos_window_ce0),      // input wire ena
  .wea('d0),      // input wire [0 : 0] wea
  .addra(cos_window_address0),  // input wire [11 : 0] addra
  .dina('d0),    // input wire [31 : 0] dina
  .douta(cos_window_q0)  // output wire [31 : 0] douta
);

scale_model_sz scale_model_sz_inst (
  .clka(clk_100),    // input wire clka
  .ena(scale_model_sz_ce0),      // input wire ena
  .wea(scale_model_sz_we0),      // input wire [0 : 0] wea
  .addra(scale_model_sz_address0),  // input wire [2 : 0] addra
  .dina(scale_model_sz_d0),    // input wire [31 : 0] dina
  .douta(scale_model_sz_q0),  // output wire [31 : 0] douta
  
  .clkb(clk_100),    // input wire clkb
  .enb(scale_model_sz_ce1),      // input wire enb
  .web(scale_model_sz_we1),      // input wire [0 : 0] web
  .addrb(scale_model_sz_address1),  // input wire [2 : 0] addrb
  .dinb(scale_model_sz_d1),    // input wire [31 : 0] dinb
  .doutb(scale_model_sz_q1)  // output wire [31 : 0] doutb
);

scale_factor scale_factor_inst (
  .clka(clk_100),    // input wire clka
  .ena(scale_factor_ce0),      // input wire ena
  .wea(scale_factor_we0),      // input wire [0 : 0] wea
  .addra(scale_factor_address0),  // input wire [0 : 0] addra
  .dina(scale_factor_d0),    // input wire [31 : 0] dina
  .douta(scale_factor_q0),  // output wire [31 : 0] douta
  
  .clkb(clk_100),    // input wire clkb
  .enb(scale_factor_ce1),      // input wire enb
  .web(scale_factor_we1),      // input wire [0 : 0] web
  .addrb(scale_factor_address1),  // input wire [0 : 0] addrb
  .dinb(scale_factor_d1),    // input wire [31 : 0] dinb
  .doutb(scale_factor_q1)  // output wire [31 : 0] doutb
);

sf_num_real sf_num_real_inst (
  .clka(clk_100),    // input wire clka
  .ena(sf_num_real_ce0),      // input wire ena
  .wea(sf_num_real_we0),      // input wire [0 : 0] wea
  .addra(sf_num_real_address0),  // input wire [13 : 0] addra
  .dina(sf_num_real_d0),    // input wire [31 : 0] dina
  .douta(sf_num_real_q0)  // output wire [31 : 0] douta
);

sf_num_imag sf_num_imag_inst (
  .clka(clk_100),    // input wire clka
  .ena(sf_num_imag_ce0),      // input wire ena
  .wea(sf_num_imag_we0),      // input wire [0 : 0] wea
  .addra(sf_num_imag_address0),  // input wire [13 : 0] addra
  .dina(sf_num_imag_d0),    // input wire [31 : 0] dina
  .douta(sf_num_imag_q0)  // output wire [31 : 0] douta
);

sf_den sf_den_inst (
  .clka(clk_100),    // input wire clka
  .ena(sf_den_ce0),      // input wire ena
  .wea(sf_den_we0),      // input wire [0 : 0] wea
  .addra(sf_den_address0),  // input wire [4 : 0] addra
  .dina(sf_den_d0),    // input wire [31 : 0] dina
  .douta(sf_den_q0)  // output wire [31 : 0] douta
);

 /**********************************HDMI输出部分***************************************/
wire [23:0]hdmi_out_data;
wire hdmi_out_hs,hdmi_out_vs,hdmi_out_de;

 HDMI_OUT HDMI_OUT_inst(
      .s_rst_n(s_rst_n),          
                         
      .hdmi_data(hdmi_data),     
      .hdmi_pclk(hdmi_pclk),     
      .hdmi_hs(hdmi_hs),       
      .hdmi_vs(hdmi_vs),       
      .hdmi_de(hdmi_de),       
      .hdmi_vs_sel(hdmi_vs_sel),      
                         
      .result_x(result_x),         
      .result_y(result_y),         
      .result_w(result_w),  
      .result_h(result_h),  
                      
      .hdmi_out_data(hdmi_out_data),     
      .hdmi_out_hs(hdmi_out_hs),       
      .hdmi_out_vs(hdmi_out_vs),       
      .hdmi_out_de(hdmi_out_de)
 );
  /**********************************输出IP核***************************************/
 uihdmitx_0 uihdmitx_inst(
    .RSTn_i(locked&&cfg_done),
    .HS_i(hdmi_out_hs),
    .VS_i(hdmi_out_vs),
    .VDE_i(hdmi_out_de),
    .RGB_i(hdmi_out_data),
    .PCLKX1_i(pclkx1),
    .PCLKX2_5_i(1'b0),
    .PCLKX5_i(pclkx5),
    .TMDS_TX_CLK_P(HDMI_CLK_P),
    .TMDS_TX_CLK_N(HDMI_CLK_N), 
    .TMDS_TX_P(HDMI_TX_P),
    .TMDS_TX_N(HDMI_TX_N));
    
 uidelay_0 uidelay_inst (
     .clk_i(clk_100),    // input wire clk_i
     .rstn_i(1'b1),  // input wire rstn_i
      .rst_o(rst_o) );  // output wire rst_o
 /*************************************************************************/
  
ila_0 ila_inst (
	.clk(clk_100), // input wire clk

	.probe0(hls_pos_x), // input wire [15:0]  probe0  
	.probe1(hls_pos_y), // input wire [15:0]  probe1 
	.probe2(hls_pos_w), // input wire [15:0]  probe2 
	.probe3(hls_pos_h), // input wire [15:0]  probe3 
	.probe4(result_x), // input wire [15:0]  probe4 
	.probe5(result_y), // input wire [15:0]  probe5 
	.probe6(result_w), // input wire [15:0]  probe6 
	.probe7(result_h), // input wire [15:0]  probe7 
	.probe8(ap_start), // input wire [0:0]  probe8 
	.probe9(ap_done), // input wire [0:0]  probe9 
	.probe10(hdmi_in_vs), // input wire [0:0]  probe10
	.probe11(hdmi_en), // input wire [0:0]  probe11
	.probe12(agg_result_x_ap_vld), // input wire [0:0]  probe12 
	.probe13(agg_result_y_ap_vld), // input wire [0:0]  probe13 
	.probe14(agg_result_width_ap_vld), // input wire [0:0]  probe14 
	.probe15(agg_result_height_ap_vld), // input wire [0:0]  probe15 
	.probe16(agg_result_x), // input wire [15:0]  probe16 
	.probe17(agg_result_y), // input wire [15:0]  probe17 
	.probe18(agg_result_width), // input wire [15:0]  probe18 
	.probe19(agg_result_height) // input wire [15:0]  probe19
);
  /*************************************************************************/
endmodule
