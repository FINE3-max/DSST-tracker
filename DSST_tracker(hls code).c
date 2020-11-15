#include"KCF_tracker.h"
/*
 输入：
	img  			150*300图像矩阵；
	res_in   	    第一帧数据；
	frame			首帧标志（高有效）；
	model_xf		首帧的傅里叶域HOG特征值；
	model_alphaf	首帧训练的跟踪分类器
	yf				高斯型回归标签
	cos_window		余弦窗口（汉宁窗）
	scale_model_sz  初始化所用数据
	scale_factor    最大/小尺度因子
	sf_num          尺度分类器分子
	sf_den          尺度分类器分母
 输出：
 	 图像矩阵中的目标中心点位置
*/

result KCF_tracker(DATA_TYPE img[imgH * imgW], result res_in, bool frame, cmpx model_xf[feature_h][feature_w][5], float model_alphaf[feature_h][feature_w], float yf[feature_h][feature_w], float cos_window[feature_h][feature_w], int scale_model_sz[4], float scale_factor[2], cmpx sf_num[templong][nScales], float sf_den[nScales]) {
	int i, j, k, temp_int, target_h, target_w;
	int SmallTarget;
	float tempf, output_sigma, temp, cell_size;
	position pos;
	result res;
	DATA_TYPE patch[window_high][window_wide];
	cmpx zf[feature_h][feature_w][5], kzf[feature_h][feature_w];
	size scale_model_temp;
	cmpx xsf[templong][nScales];
	short int recovered_scale;
	float currentScaleFactor;

	pos.x = res_in.x;
	pos.y = res_in.y;
	target_h = res_in.height;
	target_w = res_in.width;
	currentScaleFactor = res_in.csf;
	

	//判断目标大小
	if (target_h < 32 || target_w < 64) {
		SmallTarget = 1;
		cell_size = 1.0;
//		if (target_h > 100 || target_w > 100) {
//			SmallTarget = 2;
//			cell_size = 4.0;
//		}
	}
	else {
		SmallTarget = 0;
		cell_size = 2.0;
	}

	if (frame) {
		init(target_h, target_w, scale_model_sz, scale_factor);
        // 截取固定大小的子窗口（128*128或64*64）
	    get_subwindow(img, pos, patch, SmallTarget);
		// 计算子窗口的HOG特征，并进行2维fft，转换为傅里叶域
		get_hog(patch, cos_window, zf, SmallTarget);
		linear_correlation(zf, zf, kzf); //首帧对傅里叶域的特征值做自相关
		get_model(yf, kzf, zf, model_alphaf, model_xf);  //利用脊回归更新分类器alphaf和模板xf

		currentScaleFactor = 1;
		get_hog_scale(img, res_in, scale_model_sz, currentScaleFactor, xsf);
		creatnew_sf_num_den(scale_model_sz, xsf, sf_num, sf_den, frame);

	}
	else {
		// 截取固定大小的子窗口（128*128或64*64）
		get_subwindow(img, pos, patch, SmallTarget);
		// 计算子窗口的HOG特征，并进行2维fft，转换为傅里叶域
		get_hog(patch, cos_window, zf, SmallTarget);
		linear_correlation(zf, model_xf, kzf);  //当前傅里叶域的特征值与之前帧的模板xf做相关
		pos = get_response_pos(model_alphaf, kzf, pos, SmallTarget);  //使用分类器快速检测内核，得到响应最大值点

		get_subwindow(img, pos, patch, SmallTarget);
		get_hog(patch, cos_window, zf, SmallTarget);
		linear_correlation(zf, zf, kzf); //首帧对傅里叶域的特征值做自相关
		update_model(yf, kzf, zf, model_alphaf, model_xf);

		res_in.x = pos.x;
		res_in.y = pos.y;
		get_hog_scale(img, res_in, scale_model_sz, currentScaleFactor, xsf);
		recovered_scale = get_response_scale(scale_model_sz, sf_num, xsf, sf_den);

		currentScaleFactor = currentScaleFactor * scaleFactors[recovered_scale];
		if (currentScaleFactor < scale_factor[0]) currentScaleFactor = scale_factor[0];
		else if (currentScaleFactor > scale_factor[1]) currentScaleFactor = scale_factor[1];

		creatnew_sf_num_den(scale_model_sz, xsf, sf_num, sf_den, frame);
	}
	

	KCF_tracker_label3:
	for (i = 0; i < imgH * imgW; i++) {  //将img置1,方便IP核外部计算
		img[i] = 1;
	}
		
	res.x = pos.x;
	res.y = pos.y;
	res.width = floor(scale_model_sz[2] * currentScaleFactor);  //base target_w
	res.height = floor(scale_model_sz[3] * currentScaleFactor);
	res.csf = currentScaleFactor;

	return res;
}


/*
 * 输入：
 * 		img 			150*300图像矩阵；
 * 		patch_pos		目标中心点在图像矩阵中的坐标；
 * 		SmallTarget		小目标标志符（高有效）；
 * 输出：
 * 		patch			截取的固定大小子窗口（128*128或64*64）
*/
void get_subwindow(DATA_TYPE img[imgH * imgW], position patch_pos, DATA_TYPE patch[window_high][window_wide], int SmallTarget) {
	int i, j, x_low, y_low, ii, jj, ti, tj, window_h, window_w, tempi, tempj;
	DATA_TYPE temp;
	float l;

	//区分大小目标
	if (SmallTarget == 1) {
		window_h = 64;
		window_w = 64;
	}
//	else if(SmallTarget == 2){
//		window_h = 256;
//		window_w = 256;
//	}
	else {
		window_h = 128;
		window_w = 128;
	}

	//x,y方向上的起始位置
	y_low = patch_pos.y - (window_h >> 1) + 1;
	x_low = patch_pos.x - (window_w >> 1) + 1;

get_subwindow_label0:
	for (i = 0; i < window_h; i++) {
	get_subwindow_label1:
		for (j = 0; j < window_w; j++) {
			tempi = y_low + i;
			tempj = x_low + j;

			if (tempi < 0) ii = 0;
			else if (tempi > imgH - 1) ii = imgH - 1; //如果目标在边缘，超出图像范围，则重复取边缘
			else ii = tempi;

			if (tempj < 0) jj = 0;
			else if (tempj > imgW - 1) jj = imgW - 1;
			else jj = tempj;

			ti = imgW * ii;
			tj = ti + jj;
			temp = img[tj];
			patch[i][j] = temp;
		}
	}
}


/*
 * 输入：
 * 		patch 			搜索窗；
 * 		cos_window		余弦窗，用以对HOG特征值进行预处理
 * 		SmallTarget		小目标标志符（高有效）
 * 输出：
 * 		zf				傅里叶域的5维HOG特征矩阵
*/
void get_hog(DATA_TYPE patch[window_high][window_wide], float cos_window[feature_h][feature_w], cmpx zf[feature_h][feature_w][5], int SmallTarget) {
	int i, j, block, x_end, y_end, x, y, ii, jj, gs, go, binx, biny, binz, idx, idy, addr, temp_int, gg, window_h, window_w, cell_size, k;
	float tempy, tempx, temp, Iphase, temp1, temp2, temp3, temps, tt, temp0, temp01, temp02, temp03, temp4, temp04, temp5, temp05;
	DATA_TYPE Ied[window_high][window_wide], gradorient[window_high][window_wide];
	HOG_TYPE cell[feature_h][feature_w][nOrients], hist3dbig[4][4][7] = { 0 };
	cmpx reg;
	float dft_0[2 * feature_h], dft_1[2 * feature_h], dft_2[2 * feature_h], dft_3[2 * feature_h], dft_4[2 * feature_h];
	float dft2_0[2 * feature_h], dft2_1[2 * feature_h], dft2_2[2 * feature_h], dft2_3[2 * feature_h], dft2_4[2 * feature_h];

	if (SmallTarget == 1) {
		window_h = 64;
		window_w = 64;
		block = 2;
		cell_size = 1;
	}
//	else if (SmallTarget == 2) {
//		window_h = 256;
//		window_w = 256;
//		block = 8;
//		cell_size = 4;
//	}
	else {
		window_h = 128;
		window_w = 128;
		block = 4;
		cell_size = 2;
	}

	//通过卷积，求图像块的梯度矩阵Ied及梯度方向矩阵gradorient
get_hog_label0:
	for (i = 0; i < window_h; i++) {
	get_hog_label1:
		for (j = 0; j < window_w; j++) {
			if (i == 0) {
				if (j == 0) {
					tempy = patch[i][j + 1];
					temp = tempy * tempy;

				}
				else if (j == window_w - 1) {
					tempy = -patch[i][j - 1];
					temp = tempy * tempy;

				}
				else {
					temp1 = patch[i][j + 1];
					temp2 = patch[i][j - 1];
					tempy = temp1 - temp2;
					temp = tempy * tempy;

				}
				temp1 = patch[(i + 1)][j];
				temp2 = patch[(i + 1)][j];
				tempx = temp1 * temp2;
				tt = tempx + temp;
				temps = sqrt(tt);
				Ied[i][j] = temps;
				temp_int = (int)temps;
				Ied[i][j] = temp_int;
				if (tt) Iphase = tempy / temp_int;
				else Iphase = 0;
				temp = atanX(Iphase) + PI / 2;
				gradorient[i][j] = (int)temp;

				continue;
			}
			if (i == window_h - 1) {
				if (j == 0) {
					tempy = patch[i][j + 1];
					temp = tempy * tempy;

				}
				else if (j == window_w - 1) {
					tempy = -patch[i][j - 1];
					temp = tempy * tempy;

				}
				else {
					temp1 = patch[i][j + 1];
					temp2 = patch[i][j - 1];
					tempy = temp1 - temp2;
					temp = tempy * tempy;

				}
				tempx = patch[(i - 1)][j] * patch[(i - 1)][j];
				tt = tempx + temp;
				temps = sqrt(tt);
				Ied[i][j] = temps;
				temp_int = (int)temps;
				Ied[i][j] = temp_int;
				if (tt) Iphase = tempy / temp_int;
				else Iphase = 0;
				temp = atanX(Iphase) + PI / 2;
				gradorient[i][j] = (int)temp;

				continue;
			}
			if (j == 0) {
				tempy = patch[i][j + 1];
				temp = tempy * tempy;
				temp1 = patch[(i - 1)][j];
				temp2 = patch[(i + 1)][j];
				tempx = temp1 - temp2;
				tempx = tempx * tempx;
				tt = tempx + temp;
				temps = sqrt(tt);
				Ied[i][j] = temps;
				temp_int = (int)temps;
				Ied[i][j] = temp_int;
				if (tt) Iphase = tempy / temp_int;
				else Iphase = 0;
				temp = atanX(Iphase) + PI / 2;
				gradorient[i][j] = (int)temp;

				continue;
			}
			if (j == window_w - 1) {
				tempy = -patch[i][j - 1];
				temp = tempy * tempy;
				temp1 = patch[(i - 1)][j];
				temp2 = patch[(i + 1)][j];
				tempx = temp1 - temp2;
				tempx = tempx * tempx;
				tt = tempx + temp;
				temps = sqrt(tt);
				Ied[i][j] = temps;
				temp_int = (int)temps;
				Ied[i][j] = temp_int;
				if (tt) Iphase = tempy / temp_int;
				else Iphase = 0;
				temp = atanX(Iphase) + PI / 2;
				gradorient[i][j] = (int)temp;

				continue;
			}
			temp1 = patch[i][j + 1];
			temp2 = patch[i][j - 1];
			tempy = temp1 - temp2;
			temp = tempy * tempy;
			temp1 = patch[(i - 1)][j];
			temp2 = patch[(i + 1)][j];
			tempx = temp1 - temp2;
			tempx = tempx * tempx;
			tt = tempx + temp;
			temps = sqrt(tt);
			Ied[i][j] = temps;
			temp_int = (int)temps;
			Ied[i][j] = temp_int;
			if (tt) Iphase = tempy / temp_int;
			else Iphase = 0;
			temp = atanX(Iphase) + PI / 2;
			gradorient[i][j] = (int)temp;

		}
	}
	x_end = window_w - block + 1;  //x方向块左角能达到的最大值
	y_end = window_h - block + 1;
	//计算hog特征
get_hog_label2:
	for (y = 0; y < y_end; y += block) {
	get_hog_label3:
		for (x = 0; x < x_end; x += block) {
		get_hog_label4:
			for (i = 0; i < block; i++) {
			get_hog_label5:
				for (j = 0; j < block; j++) {
					ii = y + i;  //在整个坐标系中的坐标
					jj = x + j;
					gs = Ied[ii][jj];  //梯度值
					go = gradorient[ii][jj];  //梯度方向
					// 计算八个统计区间中心点的坐标
					binx = (j + (cell_size >> 1)) / cell_size;
					biny = (i + (cell_size >> 1)) / cell_size;
					temp = nOrients;
					tempx = PI / temp;
					temp = go;
					temp = (temp + tempx * 0.5) / tempx;
					binz = (int)temp;
					if (gs == 0) continue;

					//梯度映射
					addr = hist3dbig[biny][binx][binz];
					hist3dbig[biny][binx][binz] = gs + addr;

					addr = hist3dbig[biny][binx][binz + 1];
					hist3dbig[biny][binx][binz + 1] = gs + addr;

					addr = hist3dbig[biny][binx + 1][binz];
					hist3dbig[biny][binx + 1][binz] = gs + addr;

					addr = hist3dbig[biny][binx + 1][binz + 1];
					hist3dbig[biny][binx + 1][binz + 1] = gs + addr;

					addr = hist3dbig[biny + 1][binx][binz];
					hist3dbig[biny + 1][binx][binz] = gs + addr;

					addr = hist3dbig[biny + 1][binx][binz + 1];
					hist3dbig[biny + 1][binx][binz + 1] = gs + addr;

					addr = hist3dbig[biny + 1][binx + 1][binz];
					hist3dbig[biny + 1][binx + 1][binz] = gs + addr;

					addr = hist3dbig[biny + 1][binx + 1][binz + 1];
					hist3dbig[biny + 1][binx + 1][binz + 1] = gs + addr;
				}
			}

			if (SmallTarget == 1) {
				idx = x >> 1;
				idy = y >> 1;
			}
//			else if (SmallTarget == 2) {
//				idx = x >> 3;
//				idy = y >> 3;
//			}
			else {
				idx = x >> 2;
				idy = y >> 2;
			}

			cell[idy << 1][idx << 1][4] = (patch[y][x] + patch[y + 1][x] + patch[y + 1][x + 1] + patch[y][x + 1]) >> 2;
			cell[(idy << 1) + 1][idx << 1][4] = (patch[y + 2][x] + patch[y + 3][x] + patch[y + 3][x + 1] + patch[y + 2][x + 1]) >> 2;
			cell[(idy << 1) + 1][(idx << 1) + 1][4] = (patch[y + 2][x + 2] + patch[y + 3][x + 2] + patch[y + 3][x + 3] + patch[y + 2][x + 3]) >> 2;
			cell[idy << 1][(idx << 1) + 1][4] = (patch[y][x + 2] + patch[y + 1][x + 2] + patch[y + 1][x + 3] + patch[y][x + 3]) >> 2;

		get_hog_label6:
			for (i = 1; i < nOrients; i++) {  //舍去最后一维(全0)
				cell[idy << 1][idx << 1][i - 1] = hist3dbig[1][1][i];
				cell[(idy << 1) + 1][idx << 1][i - 1] = hist3dbig[2][1][i];
				cell[(idy << 1) + 1][(idx << 1) + 1][i - 1] = hist3dbig[2][2][i];
				cell[idy << 1][(idx << 1) + 1][i - 1] = hist3dbig[1][2][i];
			}
			memset(hist3dbig, 0, sizeof(hist3dbig));
		}
	}
	//与cos_window相应位置相乘，对5维矩阵cell的每一维分别做fft
	//对列做fft
	get_hog_label7:
		for (j = 0; j < feature_w; j = j + 2) {
		get_hog_label8:
			for (i = 0; i < feature_h; i++) {
				temp = cell[i][j][0];
				temp = temp * cos_window[i][j];
				dft_0[i << 1] = temp;
				dft_0[(i << 1) + 1] = 0;

				temp1 = cell[i][j][1];
				temp1 = temp1 * cos_window[i][j];
				dft_1[i << 1] = temp1;
				dft_1[(i << 1) + 1] = 0;

				temp2 = cell[i][j][2];
				temp2 = temp2 * cos_window[i][j];
				dft_2[i << 1] = temp2;
				dft_2[(i << 1) + 1] = 0;

				temp = cell[i][j][3];
				temp = temp * cos_window[i][j];
				dft_3[i << 1] = temp;
				dft_3[(i << 1) + 1] = 0;

				temp1 = cell[i][j][4];
				temp1 = temp1 * cos_window[i][j];
				dft_4[i << 1] = temp1;
				dft_4[(i << 1) + 1] = 0;

				temp = cell[i][j + 1][0];
				temp = temp * cos_window[i][j];
				dft2_0[i << 1] = temp;
				dft2_0[(i << 1) + 1] = 0;

				temp1 = cell[i][j + 1][1];
				temp1 = temp1 * cos_window[i][j];
				dft2_1[i << 1] = temp1;
				dft2_1[(i << 1) + 1] = 0;

				temp2 = cell[i][j + 1][2];
				temp2 = temp2 * cos_window[i][j];
				dft2_2[i << 1] = temp2;
				dft2_2[(i << 1) + 1] = 0;

				temp = cell[i][j + 1][3];
				temp = temp * cos_window[i][j];
				dft2_3[i << 1] = temp;
				dft2_3[(i << 1) + 1] = 0;

				temp1 = cell[i][j + 1][4];
				temp1 = temp1 * cos_window[i][j];
				dft2_4[i << 1] = temp1;
				dft2_4[(i << 1) + 1] = 0;

			}
			fft_8(dft_0, 0);
			fft_8(dft_1, 0);
			fft_8(dft_2, 0);
			fft_8(dft_3, 0);
			fft_8(dft_4, 0);

			fft_8(dft2_0, 0);
			fft_8(dft2_1, 0);
			fft_8(dft2_2, 0);
			fft_8(dft2_3, 0);
			fft_8(dft2_4, 0);
	

		get_hog_label9:
			for (i = 0; i < feature_h; i++) {
				reg.imag = dft_0[(i << 1) + 1];
				reg.real = dft_0[i << 1];
				zf[i][j][0].imag = reg.imag;
				zf[i][j][0].real = reg.real;

				reg.imag = dft_1[(i << 1) + 1];
				reg.real = dft_1[i << 1];
				zf[i][j][1].imag = reg.imag;
				zf[i][j][1].real = reg.real;

				reg.imag = dft_2[(i << 1) + 1];
				reg.real = dft_2[i << 1];
				zf[i][j][2].imag = reg.imag;
				zf[i][j][2].real = reg.real;

				reg.imag = dft_3[(i << 1) + 1];
				reg.real = dft_3[i << 1];
				zf[i][j][3].imag = reg.imag;
				zf[i][j][3].real = reg.real;

				reg.imag = dft_4[(i << 1) + 1];
				reg.real = dft_4[i << 1];
				zf[i][j][4].imag = reg.imag;
				zf[i][j][4].real = reg.real;

				reg.imag = dft2_0[(i << 1) + 1];
				reg.real = dft2_0[i << 1];
				zf[i][j + 1][0].imag = reg.imag;
				zf[i][j + 1][0].real = reg.real;

				reg.imag = dft2_1[(i << 1) + 1];
				reg.real = dft2_1[i << 1];
				zf[i][j + 1][1].imag = reg.imag;
				zf[i][j + 1][1].real = reg.real;

				reg.imag = dft2_2[(i << 1) + 1];
				reg.real = dft2_2[i << 1];
				zf[i][j + 1][2].imag = reg.imag;
				zf[i][j + 1][2].real = reg.real;

				reg.imag = dft2_3[(i << 1) + 1];
				reg.real = dft2_3[i << 1];
				zf[i][j + 1][3].imag = reg.imag;
				zf[i][j + 1][3].real = reg.real;

				reg.imag = dft2_4[(i << 1) + 1];
				reg.real = dft2_4[i << 1];
				zf[i][j + 1][4].imag = reg.imag;
				zf[i][j + 1][4].real = reg.real;

			}
		}
		//对行做dft
	get_hog_label10:
		for (i = 0; i < feature_h; i = i + 2) {
		get_hog_label11:
			for (j = 0; j < feature_w; j++) {
				reg.real = zf[i][j][0].real;
				reg.imag = zf[i][j][0].imag;
				dft_0[j << 1] = reg.real;
				dft_0[(j << 1) + 1] = reg.imag;

				reg.real = zf[i][j][1].real;
				reg.imag = zf[i][j][1].imag;
				dft_1[j << 1] = reg.real;
				dft_1[(j << 1) + 1] = reg.imag;

				reg.real = zf[i][j][2].real;
				reg.imag = zf[i][j][2].imag;
				dft_2[j << 1] = reg.real;
				dft_2[(j << 1) + 1] = reg.imag;

				reg.real = zf[i][j][3].real;
				reg.imag = zf[i][j][3].imag;
				dft_3[j << 1] = reg.real;
				dft_3[(j << 1) + 1] = reg.imag;

				reg.real = zf[i][j][4].real;
				reg.imag = zf[i][j][4].imag;
				dft_4[j << 1] = reg.real;
				dft_4[(j << 1) + 1] = reg.imag;

				reg.real = zf[i + 1][j][0].real;
				reg.imag = zf[i + 1][j][0].imag;
				dft2_0[j << 1] = reg.real;
				dft2_0[(j << 1) + 1] = reg.imag;

				reg.real = zf[i + 1][j][1].real;
				reg.imag = zf[i + 1][j][1].imag;
				dft2_1[j << 1] = reg.real;
				dft2_1[(j << 1) + 1] = reg.imag;

				reg.real = zf[i + 1][j][2].real;
				reg.imag = zf[i + 1][j][2].imag;
				dft2_2[j << 1] = reg.real;
				dft2_2[(j << 1) + 1] = reg.imag;

				reg.real = zf[i + 1][j][3].real;
				reg.imag = zf[i + 1][j][3].imag;
				dft2_3[j << 1] = reg.real;
				dft2_3[(j << 1) + 1] = reg.imag;

				reg.real = zf[i + 1][j][4].real;
				reg.imag = zf[i + 1][j][4].imag;
				dft2_4[j << 1] = reg.real;
				dft2_4[(j << 1) + 1] = reg.imag;

			}
			fft_8(dft_0, 0);
			fft_8(dft_1, 0);
			fft_8(dft_2, 0);
			fft_8(dft_3, 0);
			fft_8(dft_4, 0);

			fft_8(dft2_0, 0);
			fft_8(dft2_1, 0);
			fft_8(dft2_2, 0);
			fft_8(dft2_3, 0);
			fft_8(dft2_4, 0);
			
		get_hog_label12:
			for (j = 0; j < feature_w; j++) {
				reg.real = dft_0[j << 1];
				reg.imag = dft_0[(j << 1) + 1];
				zf[i][j][0].imag = reg.imag;
				zf[i][j][0].real = reg.real;

				reg.real = dft_1[j << 1];
				reg.imag = dft_1[(j << 1) + 1];
				zf[i][j][1].imag = reg.imag;
				zf[i][j][1].real = reg.real;

				reg.real = dft_2[j << 1];
				reg.imag = dft_2[(j << 1) + 1];
				zf[i][j][2].imag = reg.imag;
				zf[i][j][2].real = reg.real;

				reg.real = dft_3[j << 1];
				reg.imag = dft_3[(j << 1) + 1];
				zf[i][j][3].imag = reg.imag;
				zf[i][j][3].real = reg.real;

				reg.real = dft_4[j << 1];
				reg.imag = dft_4[(j << 1) + 1];
				zf[i][j][4].imag = reg.imag;
				zf[i][j][4].real = reg.real;

				reg.real = dft2_0[j << 1];
				reg.imag = dft2_0[(j << 1) + 1];
				zf[i + 1][j][0].imag = reg.imag;
				zf[i + 1][j][0].real = reg.real;

				reg.real = dft2_1[j << 1];
				reg.imag = dft2_1[(j << 1) + 1];
				zf[i + 1][j][1].imag = reg.imag;
				zf[i + 1][j][1].real = reg.real;

				reg.real = dft2_2[j << 1];
				reg.imag = dft2_2[(j << 1) + 1];
				zf[i + 1][j][2].imag = reg.imag;
				zf[i + 1][j][2].real = reg.real;

				reg.real = dft2_3[j << 1];
				reg.imag = dft2_3[(j << 1) + 1];
				zf[i + 1][j][3].imag = reg.imag;
				zf[i + 1][j][3].real = reg.real;

				reg.real = dft2_4[j << 1];
				reg.imag = dft2_4[(j << 1) + 1];
				zf[i + 1][j][4].imag = reg.imag;
				zf[i + 1][j][4].real = reg.real;

			}
		}
	}


/*
 * 输入：
 * 		zf 				傅里叶域的HOG特征矩阵；
 * 		model_xf		之前帧的傅里叶域的HOG特征矩阵；
 * 输出：
 * 		kzf				输入矩阵线性相关结果；
*/
void linear_correlation(cmpx zf[feature_h][feature_w][5], cmpx model_xf[feature_h][feature_w][5], cmpx kzf[feature_h][feature_w])
{
	int i, j;
	cmpx temp0, temp1, temp2, sum, temp00, temp11, temp22, temp3, temp4, temp33, temp44;
	float num;

	num = (float)(feature_w * feature_h * 5);

	//做点积的线性核
linear_correlation_label0:
	for (i = 0; i < feature_h; i++) {
		linear_correlation_label1:
		for (j = 0; j < feature_w; j++) {
			temp0.real = zf[i][j][0].real * model_xf[i][j][0].real;
			temp0.imag = model_xf[i][j][0].real * zf[i][j][0].imag;
			temp00.real = zf[i][j][0].imag * model_xf[i][j][0].imag;
			temp00.imag = zf[i][j][0].real * model_xf[i][j][0].imag;
			temp0.real = temp0.real + temp00.real;
			temp0.imag = temp0.imag - temp00.imag;

			temp1.real = zf[i][j][1].real * model_xf[i][j][1].real;
			temp1.imag = model_xf[i][j][1].real * zf[i][j][1].imag;
			temp11.real = zf[i][j][1].imag * model_xf[i][j][1].imag;
			temp11.imag = zf[i][j][1].real * model_xf[i][j][1].imag;
			temp1.real = temp1.real + temp11.real;
			temp1.imag = temp1.imag - temp11.imag;

			temp2.real = zf[i][j][2].real * model_xf[i][j][2].real;
			temp2.imag = model_xf[i][j][2].real * zf[i][j][2].imag;
			temp22.real = zf[i][j][2].imag * model_xf[i][j][2].imag;
			temp22.imag = zf[i][j][2].real * model_xf[i][j][2].imag;
			temp2.real = temp2.real + temp22.real;
			temp2.imag = temp2.imag - temp22.imag;

			temp3.real = zf[i][j][3].real * model_xf[i][j][3].real;
			temp3.imag = model_xf[i][j][3].real * zf[i][j][3].imag;
			temp33.real = zf[i][j][3].imag * model_xf[i][j][3].imag;
			temp33.imag = zf[i][j][3].real * model_xf[i][j][3].imag;
			temp3.real = temp3.real + temp33.real;
			temp3.imag = temp3.imag - temp33.imag;

			temp4.real = zf[i][j][4].real * model_xf[i][j][4].real;
			temp4.imag = model_xf[i][j][4].real * zf[i][j][4].imag;
			temp44.real = zf[i][j][4].imag * model_xf[i][j][4].imag;
			temp44.imag = zf[i][j][4].real * model_xf[i][j][4].imag;
			temp4.real = temp4.real + temp44.real;
			temp4.imag = temp4.imag - temp44.imag;

			sum.real = temp0.real + temp1.real + temp2.real + temp3.real + temp4.real;
			sum.imag = temp0.imag + temp1.imag + temp2.imag + temp3.imag + temp4.imag;

			kzf[i][j].real = sum.real / num;
			kzf[i][j].imag = sum.imag / num;
		}
	}
}


/*
 * 输入：
 * 		model_alphaf 	分类器；
 * 		kzf				当前帧与模板特征值的相关结果；
 * 		SmallTarget		小目标标志符（高有效）；
 * 输出：
 * 		patch_pos		目标位置
*/
position get_response_pos(float model_alphaf[feature_h][feature_w], cmpx kzf[feature_h][feature_w], position patch_pos, int SmallTarget) {
	int i, j, cell_size;
	float max;
	float dft_in[2 * feature_h], dft_in2[2 * feature_h], dft_in3[2 * feature_h], dft_in4[2 * feature_h];
	cmpx temp, temp2, temp3, temp4, dft_temp[feature_h][feature_w], temp0, temp20, temp30, temp40;
	position pos;

	if (SmallTarget) cell_size = 1;
//	else if (SmallTarget == 2) cell_size = 4;
	else cell_size = 2;

	//使用分类器快速检测，得到响应矩阵
get_response_pos_label0:
	for (j = 0; j < feature_w; j = j + 4) {
	get_response_pos_label1:
		for (i = 0; i < feature_h; i++) {
			temp.real = model_alphaf[i][j] * kzf[i][j].real;
			temp.imag = model_alphaf[i][j] * kzf[i][j].imag;

			dft_in[i << 1] = temp.real;
			dft_in[(i << 1) + 1] = temp.imag;

			temp.real = model_alphaf[i][j + 1] * kzf[i][j + 1].real;
			temp.imag = model_alphaf[i][j + 1] * kzf[i][j + 1].imag;

			dft_in2[i << 1] = temp.real;
			dft_in2[(i << 1) + 1] = temp.imag;

			temp.real = model_alphaf[i][j + 2] * kzf[i][j + 2].real;
			temp.imag = model_alphaf[i][j + 2] * kzf[i][j + 2].imag;

			dft_in3[i << 1] = temp.real;
			dft_in3[(i << 1) + 1] = temp.imag;

			temp.real = model_alphaf[i][j + 3] * kzf[i][j + 3].real;
			temp.imag = model_alphaf[i][j + 3] * kzf[i][j + 3].imag;

			dft_in4[i << 1] = temp.real;
			dft_in4[(i << 1) + 1] = temp.imag;

		}
		fft_8(dft_in, 1); //ifft
		fft_8(dft_in2, 1);
		fft_8(dft_in3, 1); //ifft
		fft_8(dft_in4, 1);
	get_response_pos_label2:
		for (i = 0; i < feature_h; i++) {
			dft_temp[i][j].imag = dft_in[(i << 1) + 1];
			dft_temp[i][j].real = dft_in[i << 1];

			dft_temp[i][j + 1].imag = dft_in2[(i << 1) + 1];
			dft_temp[i][j + 1].real = dft_in2[i << 1];

			dft_temp[i][j + 2].imag = dft_in3[(i << 1) + 1];
			dft_temp[i][j + 2].real = dft_in3[i << 1];

			dft_temp[i][j + 3].imag = dft_in4[(i << 1) + 1];
			dft_temp[i][j + 3].real = dft_in4[i << 1];
		}
	}
get_response_pos_label3:
	for (i = 0; i < feature_h; i = i + 4) {
	get_response_pos_label4:
		for (j = 0; j < feature_w; j++) {
			dft_in[(j << 1) + 1] = dft_temp[i][j].imag;
			dft_in[j << 1] = dft_temp[i][j].real;

			dft_in2[(j << 1) + 1] = dft_temp[i + 1][j].imag;
			dft_in2[j << 1] = dft_temp[i + 1][j].real;

			dft_in3[(j << 1) + 1] = dft_temp[i + 2][j].imag;
			dft_in3[j << 1] = dft_temp[i + 2][j].real;

			dft_in4[(j << 1) + 1] = dft_temp[i + 3][j].imag;
			dft_in4[j << 1] = dft_temp[i + 3][j].real;
		}
		fft_8(dft_in, 1);
		fft_8(dft_in2, 1);
		fft_8(dft_in3, 1);
		fft_8(dft_in4, 1);
	get_response_pos_label5:
		//找到响应最大值位置，即为目标位移
		for (j = 0; j < feature_w; j++) {
			if ((i | j) == 0) {
				max = dft_in[j << 1];
				pos.x = j;
				pos.y = i;
			}
			else {
				if (dft_in[j << 1] > max) {
					max = dft_in[j << 1];
					pos.x = j;
					pos.y = i;
				}
				if (dft_in2[j << 1] > max) {
					max = dft_in2[j << 1];
					pos.x = j;
					pos.y = i + 1;
				}
				if (dft_in3[j << 1] > max) {
					max = dft_in[j << 1];
					pos.x = j;
					pos.y = i + 2;
				}
				if (dft_in4[j << 1] > max) {
					max = dft_in2[j << 1];
					pos.x = j;
					pos.y = i + 3;
				}
			}
		}
	}

	if (pos.y > (feature_h >> 1)) pos.y = pos.y - feature_h;
	if (pos.x > (feature_w >> 1)) pos.x = pos.x - feature_w;

	pos.x = patch_pos.x + (pos.x * cell_size);
	pos.y = patch_pos.y + (pos.y * cell_size);
	return pos;
}


/*
 * 输入：
 * 		yf 				高斯型回归标签；
 * 		kzf				使用线性核的傅里叶域脊回归结果；
 * 		zf				傅里叶域中的HOG特征矩阵；
 * 输出：
 * 		model_xf		首帧的傅里叶域中的HOG特征矩阵；
 * 		model_alphaf 	带入核脊回归公式得到的分类器；
*/
void get_model(float yf[feature_h][feature_w], cmpx kzf[feature_h][feature_w], cmpx zf[feature_h][feature_w][5], float model_alphaf[feature_h][feature_w], cmpx model_xf[feature_h][feature_w][5]) {
	int i, j;
	float alphaf, temp;
	cmpx temp1;
get_model_label0:
	for (i = 0; i < feature_h; i++) {
		get_model_label1:
		for (j = 0; j < feature_w; j++) {
			if (kzf[i][j].real != 0) {  //防止分母为0
				alphaf = yf[i][j] / kzf[i][j].real;
			}
			else {
				alphaf = yf[i][j] * 8192;
			}
			model_alphaf[i][j] = alphaf;
			model_xf[i][j][0].imag = zf[i][j][0].imag;
			model_xf[i][j][0].real = zf[i][j][0].real;
			model_xf[i][j][1].imag = zf[i][j][1].imag;
			model_xf[i][j][1].real = zf[i][j][1].real;
			model_xf[i][j][2].imag = zf[i][j][2].imag;
			model_xf[i][j][2].real = zf[i][j][2].real;
			model_xf[i][j][3].imag = zf[i][j][3].imag;
			model_xf[i][j][3].real = zf[i][j][3].real;
			model_xf[i][j][4].imag = zf[i][j][4].imag;
			model_xf[i][j][4].real = zf[i][j][4].real;
		}
	}
}


void update_model(float yf[feature_h][feature_w], cmpx kzf[feature_h][feature_w], cmpx zf[feature_h][feature_w][5], float model_alphaf[feature_h][feature_w], cmpx model_xf[feature_h][feature_w][5]) {
	int i, j;
	float alphaf, temp, temp2;
	cmpx temp1;

get_model_label0:
	for (i = 0; i < feature_h; i++) {
		get_model_label1:
		for (j = 0; j < feature_w; j++) {
			if (kzf[i][j].real != 0) {  //防止分母为0
				alphaf = yf[i][j] / kzf[i][j].real;
			}
			else {
				alphaf = yf[i][j] * 8192;
			}
			temp = alphaf * interp_factor;
			temp2 = factor_1 * model_alphaf[i][j];
			model_alphaf[i][j] = temp + temp2;


			temp = zf[i][j][0].imag * interp_factor;
			temp2 = factor_1 * model_xf[i][j][0].imag;
			model_xf[i][j][0].imag = temp + temp2;

			temp = zf[i][j][0].real * interp_factor;
			temp2 = factor_1 * model_xf[i][j][0].real;
			model_xf[i][j][0].real = temp + temp2;

			temp = zf[i][j][1].real * interp_factor;
			temp2 = factor_1 * model_xf[i][j][1].real;
			model_xf[i][j][1].real = temp + temp2;

			temp = zf[i][j][1].imag * interp_factor;
			temp2 = factor_1 * model_xf[i][j][1].imag;
			model_xf[i][j][1].imag = temp + temp2;

			temp = zf[i][j][2].real * interp_factor;
			temp2 = factor_1 * model_xf[i][j][2].real;
			model_xf[i][j][2].real = temp + temp2;

			temp = zf[i][j][2].imag * interp_factor;
			temp2 = factor_1 * model_xf[i][j][1].imag;
			model_xf[i][j][2].imag = temp + temp2;

			temp = zf[i][j][3].real * interp_factor;
			temp2 = factor_1 * model_xf[i][j][3].real;
			model_xf[i][j][3].real = temp + temp2;

			temp = zf[i][j][3].imag * interp_factor;
			temp2 = factor_1 * model_xf[i][j][3].imag;
			model_xf[i][j][3].imag = temp + temp2;

			temp = zf[i][j][4].real * interp_factor;
			temp2 = factor_1 * model_xf[i][j][4].real;
			model_xf[i][j][4].real = temp + temp2;

			temp = zf[i][j][4].imag * interp_factor;
			temp2 = factor_1 * model_xf[i][j][4].imag;
			model_xf[i][j][4].imag = temp + temp2;

		}
	}
}


//查表法计算反正切函数
float atanX(float x)
{
	float y = 1;
	int k, a, yy, flag, cmp;
	int atan_x[41] = { 0 ,41 ,82 ,123 ,164 ,205 ,246 ,287 ,328 ,369 ,410 ,451 ,492 ,532 ,573 ,614 ,655 ,696 ,737 ,778 ,819 ,860 ,901 ,942 ,983 ,1024 ,1092 ,1161 ,1229 ,1297 ,1365 ,1434 ,1502 ,1570 ,1638 ,1707 ,1775 ,1843 ,1911 ,1980 ,2048 };
	int atan_y[41] = { 0 ,41 ,82 ,123 ,163 ,204 ,245 ,285 ,325 ,365 ,404 ,443 ,482 ,521 ,559 ,597 ,634 ,671 ,708 ,744 ,779 ,814 ,849 ,883 ,917 ,950 ,1003 ,1056 ,1107 ,1156 ,1204 ,1251 ,1296 ,1340 ,1382 ,1423 ,1462 ,1501 ,1538 ,1574 ,1608 };

	a = abs((int)(x * 2048));

	if (x < 0) flag = -1;
	else flag = 1;

	atanX_label0:
	for (k = 0; k < 41; k++) {
		cmp = atan_x[k];
		if (a > cmp) {}
		else {
			y = flag * atan_y[k];
			break;
		}
	}
	y = y / 2048;
	return y;
}


//查表法计算正弦、余弦函数
float sin_cosX(float x, int flag_cos)
{
	float flag, flag_fu, y, flag_pi, kk, angle;
	int k, a, b;

	int sin_x[101] = { 0,   10,   20,   30,   40,   51,   61,   71,   82,   92,   102,   112,   123,   133,   143,   154,   164,   174,   185,   195,   206,  
		216,   227,   237,   248,   258,   269,   279,   290,   301,   312,   322,   333,   344,   355,   366,   377,   388,   399,   410,   421,   432,   443, 
		455,   466,   477,   489,   501,   512,   524,   536,   548,   559,   572,   584,   596,   608,   621,   633,   646,   658,   671,   684,   697,   711,  
		724,   738,   751,   765,   779,   794,   808,   823,   837,   853,   868,   884,   899,   916,   932,   949,   966,   984,   1002,   1021,   1040,   1060,  
		1080,   1101,   1123,   1146,   1170,   1196,   1223,   1251,   1283,   1317,   1357,   1403,   1463,   1608 };

	float result_y[101] = { 0.00000,   0.01000,   0.02000,   0.03000,   0.04000,   0.05000,   0.06000,   0.07000,   0.08000,   0.09000,   0.10000,   0.11000,   0.12000,  
		0.13000,   0.14000,   0.15000,   0.16000,   0.17000,   0.18000,   0.19000,   0.20000,   0.21000,   0.22000,   0.23000,   0.24000,   0.25000,   0.26000,   0.27000,  
		0.28000,   0.29000,   0.30000,   0.31000,   0.32000,   0.33000,   0.34000,   0.35000,   0.36000,   0.37000,   0.38000,   0.39000,   0.40000,   0.41000,   0.42000,  
		0.43000,   0.44000,   0.45000,   0.46000,   0.47000,   0.48000,   0.49000,   0.50000,   0.51000,   0.52000,   0.53000,   0.54000,   0.55000,   0.56000,   0.57000,  
		0.58000,   0.59000,   0.60000,   0.61000,   0.62000,   0.63000,   0.64000,   0.65000,   0.66000,   0.67000,   0.68000,   0.69000,   0.70000,   0.71000,   0.72000, 
		0.73000,   0.74000,   0.75000,   0.76000,   0.77000,   0.78000,   0.79000,   0.80000,   0.81000,   0.82000,   0.83000,   0.84000,   0.85000,   0.86000,   0.87000,  
		0.88000,   0.89000,   0.90000,   0.91000,   0.92000,   0.93000,   0.94000,   0.95000,   0.96000,   0.97000,   0.98000,   0.99000,   1.00000 };

	int cos_x[101] = { 0,   144,   205,   251,   290,   325,   356,   385,   412,   437,   461,   484,   506,   527,   548,   568,   587,   605,   624,   641,  
		658,   675,   692,   708,   724,   740,   755,   770,   785,   800,   814,   828,   842,   856,   870,   883,   897,   910,   923,   936,   949,   962, 
		974,   987,   999,   1012,   1024,   1036,   1048,   1060,   1072,   1084,   1095,   1107,   1119,   1130,   1141,   1153,   1164,   1175,   1187,   1198,   
		1209,   1220,   1231,   1242,   1253,   1264,   1274,   1285,   1296,   1307,   1317,   1328,   1339,   1349,   1360,   1370,   1381,   1391,   1402,   1412,   
		1423,   1433,   1443,   1454,   1464,   1474,   1485,   1495,   1505,   1516,   1526,   1536,   1547,   1557,   1567,   1577,   1588,   1598,   1608 };

	if (x < 0) {
		flag = -1;
		a = -1024 * x;
	}
	else {
		flag = 1;
		a = 1024 * x;
	}

	if (flag_cos) {
		a = a + 1608;  //0.5*PI
		flag = 1;
	}
sin_cosX_label0:
	while (a >= 6433) {  //a > 2*pi
		a = a - 6433;
	}

	if (a >= 3216) {  //a > pi
		flag_fu = -1 * flag;
		a = a - 3216;
	}
	else flag_fu = flag;

	b = a - 1608;

	if (a >= 1608) {  //a > 0.5 * pi
		if (b >= 1608) {
			angle = result_y[0];
			y = flag_fu * angle;
			return y;
		}
	sin_cosX_label1:
		for (k = 0; k < 101; k++) {
			if (b > cos_x[k]) {}
			else {
				angle = result_y[100 - k];
				y = flag_fu * angle;
				break;
			}
		}
	}
	else {
		if (a >= 1608) {
			angle = result_y[100];
			y = flag_fu * angle;
			return y;
		}
	sin_cosX_label2:
		for (k = 0; k < 101; k++) {
			if (a > sin_x[k]) {}
			else {
				angle = result_y[k];
				y = flag_fu * angle;
				break;
			}
		}
	}
	return y;
}


void fft_8(float in[feature_h << 1], bool ifft)
{
	float f[128], temp, temp0, temp1;
	int i;
	temp = feature_h;

	bitrv2(2 * feature_h, in);
	cftfsub(2 * feature_h, in);


	if (ifft) {
		in[1] = in[1] / temp;
		in[0] = in[0] / temp;
		fft_8_label0:
		for (i = 1; i < (feature_h >> 1); i++)
		{
			in[(i << 1) + 1] = in[(i << 1) + 1] / temp;
			in[i << 1] = in[i << 1] / temp;
			in[((feature_h - i) << 1) + 1] = in[((feature_h - i) << 1) + 1] / temp;
			in[(feature_h - i) << 1] = in[(feature_h - i) << 1] / temp;
			temp0 = in[(i << 1) + 1];
			temp1 = in[i << 1];
			in[(i << 1) + 1] = in[((feature_h - i) << 1) + 1];
			in[i << 1] = in[(feature_h - i) << 1];
			in[((feature_h - i) << 1) + 1] = temp0;
			in[(feature_h - i) << 1] = temp1;
		}
		in[feature_h + 1] = (in[feature_h + 1]) / temp;
		in[feature_h] = (in[feature_h]) / temp;
	}
}



void bitrv2(int n, float* a)
{
	int j0, k0, j1, k1, l, m, i, j, k;
	float xr, xi, yr, yi;

	l = 8;
	m = 8;

	j0 = 0;
bitrv2_label3:
for (k0 = 0; k0 < m; k0 += 2) {
	k = k0;
bitrv2_label2:
	for (j = j0; j < j0 + k0; j += 2) {
		xr = a[j];
		xi = a[j + 1];
		yr = a[k];
		yi = a[k + 1];
		a[j] = yr;
		a[j + 1] = yi;
		a[k] = xr;
		a[k + 1] = xi;
		j1 = j + m;
		k1 = k + 2 * m;
		xr = a[j1];
		xi = a[j1 + 1];
		yr = a[k1];
		yi = a[k1 + 1];
		a[j1] = yr;
		a[j1 + 1] = yi;
		a[k1] = xr;
		a[k1 + 1] = xi;
		j1 += m;
		k1 -= m;
		xr = a[j1];
		xi = a[j1 + 1];
		yr = a[k1];
		yi = a[k1 + 1];
		a[j1] = yr;
		a[j1 + 1] = yi;
		a[k1] = xr;
		a[k1 + 1] = xi;
		j1 += m;
		k1 += 2 * m;
		xr = a[j1];
		xi = a[j1 + 1];
		yr = a[k1];
		yi = a[k1 + 1];
		a[j1] = yr;
		a[j1 + 1] = yi;
		a[k1] = xr;
		a[k1 + 1] = xi;
	bitrv2_label4:
		for (i = n >> 1; i > (k ^= i); i >>= 1);
	}
	j1 = j0 + k0 + m;
	k1 = j1 + m;
	xr = a[j1];
	xi = a[j1 + 1];
	yr = a[k1];
	yi = a[k1 + 1];
	a[j1] = yr;
	a[j1 + 1] = yi;
	a[k1] = xr;
	a[k1 + 1] = xi;
bitrv2_label5:for (i = n >> 1; i > (j0 ^= i); i >>= 1);
}
}


void cftfsub(int n, float* a)
{
	int j, j1, j2, j3, l;
	float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

	l = 2;
	cft1st(n, a);
	l = 16;
	while ((l << 3) <= n) {
		cftmdl(n, l, a);
		l <<= 3;
	}
}


void cft1st(int n, float* a)
{
	int j, kj, kr;
	float dsp_mul1, dsp_mul2, dsp_add1, dsp_add2;
	float ew, wn4r, wtmp, wk1r, wk1i, wk2r, wk2i, wk3r, wk3i,
		wk4r, wk4i, wk5r, wk5i, wk6r, wk6i, wk7r, wk7i;
	float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
		y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
		y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i;

	wn4r = WR5000;
	x0r = a[0] + a[2];
	x0i = a[1] + a[3];
	x1r = a[0] - a[2];
	x1i = a[1] - a[3];
	x2r = a[4] + a[6];
	x2i = a[5] + a[7];
	x3r = a[4] - a[6];
	x3i = a[5] - a[7];
	y0r = x0r + x2r;
	y0i = x0i + x2i;
	y2r = x0r - x2r;
	y2i = x0i - x2i;
	y1r = x1r - x3i;
	y1i = x1i + x3r;
	y3r = x1r + x3i;
	y3i = x1i - x3r;
	x0r = a[8] + a[10];
	x0i = a[9] + a[11];
	x1r = a[8] - a[10];
	x1i = a[9] - a[11];
	x2r = a[12] + a[14];
	x2i = a[13] + a[15];
	x3r = a[12] - a[14];
	x3i = a[13] - a[15];
	y4r = x0r + x2r;
	y4i = x0i + x2i;
	y6r = x0r - x2r;
	y6i = x0i - x2i;
	x0r = x1r - x3i;
	x0i = x1i + x3r;
	x2r = x1r + x3i;
	x2i = x1i - x3r;
	dsp_add1 = x0r - x0i;
	dsp_mul1 = wn4r * dsp_add1;
	y5r = dsp_mul1;
	dsp_add1 = x0r + x0i;
	dsp_mul1 = wn4r * dsp_add1;
	y5i = dsp_mul1;
	dsp_add1 = x2r - x2i;
	dsp_mul1 = wn4r * dsp_add1;
	y7r = dsp_mul1;
	dsp_add1 = x2r + x2i;
	dsp_mul1 = wn4r * dsp_add1;
	y7i = dsp_mul1;
/*	y5r = wn4r * (x0r - x0i);
	y5i = wn4r * (x0r + x0i);
	y7r = wn4r * (x2r - x2i);
	y7i = wn4r * (x2r + x2i);
*/
	a[2] = y1r + y5r;
	a[3] = y1i + y5i;
	a[10] = y1r - y5r;
	a[11] = y1i - y5i;
	a[6] = y3r - y7i;
	a[7] = y3i + y7r;
	a[14] = y3r + y7i;
	a[15] = y3i - y7r;
	a[0] = y0r + y4r;
	a[1] = y0i + y4i;
	a[8] = y0r - y4r;
	a[9] = y0i - y4i;
	a[4] = y2r - y6i;
	a[5] = y2i + y6r;
	a[12] = y2r + y6i;
	a[13] = y2i - y6r;
	if (n > 16) {
		wk1r = WR2500;
		wk1i = WI2500;
		x0r = a[16] + a[18];
		x0i = a[17] + a[19];
		x1r = a[16] - a[18];
		x1i = a[17] - a[19];
		x2r = a[20] + a[22];
		x2i = a[21] + a[23];
		x3r = a[20] - a[22];
		x3i = a[21] - a[23];
		y0r = x0r + x2r;
		y0i = x0i + x2i;
		y2r = x0r - x2r;
		y2i = x0i - x2i;
		y1r = x1r - x3i;
		y1i = x1i + x3r;
		y3r = x1r + x3i;
		y3i = x1i - x3r;
		x0r = a[24] + a[26];
		x0i = a[25] + a[27];
		x1r = a[24] - a[26];
		x1i = a[25] - a[27];
		x2r = a[28] + a[30];
		x2i = a[29] + a[31];
		x3r = a[28] - a[30];
		x3i = a[29] - a[31];
		y4r = x0r + x2r;
		y4i = x0i + x2i;
		y6r = x0r - x2r;
		y6i = x0i - x2i;
		x0r = x1r - x3i;
		x0i = x1i + x3r;
		x2r = x1r + x3i;
		x2i = x3r - x1i;
		dsp_mul1 = wk1i * x0r;
		dsp_mul2 = wk1r * x0i;
		dsp_add1 = dsp_mul1 - dsp_mul2;
		y5r = dsp_add1;

		dsp_mul1 = wk1i * x0i;
		dsp_mul2 = wk1r * x0r;
		dsp_add1 = dsp_mul1 + dsp_mul2;
		y5i = dsp_add1;

		dsp_mul1 = wk1r * x2r;
		dsp_mul2 = wk1i * x2i;
		dsp_add1 = dsp_mul1 + dsp_mul2;
		y7r = dsp_add1;

		dsp_mul1 = wk1r * x2i;
		dsp_mul2 = wk1i * x2r;
		dsp_add1 = dsp_mul1 - dsp_mul2;
		y7i = dsp_add1;

		dsp_mul1 = wk1r * y1r;
		dsp_mul2 = wk1i * y1i;
		dsp_add1 = dsp_mul1 - dsp_mul2;
		x0r = dsp_add1;

		dsp_mul1 = wk1r * y1i;
		dsp_mul2 = wk1i * y1r;
		dsp_add1 = dsp_mul1 + dsp_mul2;
		x0i = dsp_add1;
/*		y5r = wk1i * x0r - wk1r * x0i;
		y5i = wk1i * x0i + wk1r * x0r;
		y7r = wk1r * x2r + wk1i * x2i;
		y7i = wk1r * x2i - wk1i * x2r;
		x0r = wk1r * y1r - wk1i * y1i;
		x0i = wk1r * y1i + wk1i * y1r;
*/
		a[18] = x0r + y5r;
		a[19] = x0i + y5i;
		a[26] = y5i - x0i;
		a[27] = x0r - y5r;

		dsp_mul1 = wk1i * y3r;
		dsp_mul2 = wk1r * y3i;
		dsp_add1 = dsp_mul1 - dsp_mul2;
		x0r = dsp_add1;

		dsp_mul1 = wk1i * y3i;
		dsp_mul2 = wk1r * y3r;
		dsp_add1 = dsp_mul1 + dsp_mul2;
		x0i = dsp_add1;
/*		x0r = wk1i * y3r - wk1r * y3i;
		x0i = wk1i * y3i + wk1r * y3r;
*/
		a[22] = x0r - y7r;
		a[23] = x0i + y7i;
		a[30] = y7i - x0i;
		a[31] = x0r + y7r;
		a[16] = y0r + y4r;
		a[17] = y0i + y4i;
		a[24] = y4i - y0i;
		a[25] = y0r - y4r;
		x0r = y2r - y6i;
		x0i = y2i + y6r;

		dsp_add1 = x0r - x0i;
		dsp_mul1 = wn4r * dsp_add1;
		a[20] = dsp_mul1;
	
		dsp_add1 = x0i + x0r;
		dsp_mul1 = wn4r * dsp_add1;
		a[21] = dsp_mul1;
/*		a[20] = wn4r * (x0r - x0i);
		a[21] = wn4r * (x0i + x0r);
*/
		x0r = y6r - y2i;
		x0i = y2r + y6i;

		dsp_add1 = x0r - x0i;
		dsp_mul1 = wn4r * dsp_add1;
		a[28] = dsp_mul1;

		dsp_add1 = x0i + x0r;
		dsp_mul1 = wn4r * dsp_add1;
		a[29] = dsp_mul1;
/*		a[28] = wn4r * (x0r - x0i);
		a[29] = wn4r * (x0i + x0r);
*/
		ew = M_PI_2 / n;
		kr = n >> 2;
	cft1st_label0:for (j = 32; j < n; j += 16) {
	cft1st_label1:for (kj = n >> 2; kj > (kr ^= kj); kj >>= 1);
		wk1r = sin_cosX(ew * kr, 1);//cos
		wk1i = sin_cosX(ew * kr, 0);
		//wk1r = cos(ew * kr);
		//wk1i = sin(ew * kr);
		dsp_mul1 = 2 * wk1i * wk1i;
		dsp_add1 = 1 - dsp_mul1;
		wk2r = dsp_add1;

		//wk2r = 1 - 2 * wk1i * wk1i;
/**/		wk2i = 2 * wk1i * wk1r;
		wtmp = 2 * wk2i;
		wk3r = wk1r - wtmp * wk1i;
		wk3i = wtmp * wk1r - wk1i;
		wk4r = 1 - wtmp * wk2i;
		wk4i = wtmp * wk2r;
		wtmp = 2 * wk4i;
		wk5r = wk3r - wtmp * wk1i;
		wk5i = wtmp * wk1r - wk3i;
		wk6r = wk2r - wtmp * wk2i;
		wk6i = wtmp * wk2r - wk2i;
		wk7r = wk1r - wtmp * wk3i;
		wk7i = wtmp * wk3r - wk1i;

		x0r = a[j] + a[j + 2];
		x0i = a[j + 1] + a[j + 3];
		x1r = a[j] - a[j + 2];
		x1i = a[j + 1] - a[j + 3];
		x2r = a[j + 4] + a[j + 6];
		x2i = a[j + 5] + a[j + 7];
		x3r = a[j + 4] - a[j + 6];
		x3i = a[j + 5] - a[j + 7];
		y0r = x0r + x2r;
		y0i = x0i + x2i;
		y2r = x0r - x2r;
		y2i = x0i - x2i;
		y1r = x1r - x3i;
		y1i = x1i + x3r;
		y3r = x1r + x3i;
		y3i = x1i - x3r;
		x0r = a[j + 8] + a[j + 10];
		x0i = a[j + 9] + a[j + 11];
		x1r = a[j + 8] - a[j + 10];
		x1i = a[j + 9] - a[j + 11];
		x2r = a[j + 12] + a[j + 14];
		x2i = a[j + 13] + a[j + 15];
		x3r = a[j + 12] - a[j + 14];
		x3i = a[j + 13] - a[j + 15];
		y4r = x0r + x2r;
		y4i = x0i + x2i;
		y6r = x0r - x2r;
		y6i = x0i - x2i;
		x0r = x1r - x3i;
		x0i = x1i + x3r;
		x2r = x1r + x3i;
		x2i = x1i - x3r;
		y5r = wn4r * (x0r - x0i);
		y5i = wn4r * (x0r + x0i);
		y7r = wn4r * (x2r - x2i);
		y7i = wn4r * (x2r + x2i);
		x0r = y1r + y5r;
		x0i = y1i + y5i;
		a[j + 2] = wk1r * x0r - wk1i * x0i;
		a[j + 3] = wk1r * x0i + wk1i * x0r;
		x0r = y1r - y5r;
		x0i = y1i - y5i;
		a[j + 10] = wk5r * x0r - wk5i * x0i;
		a[j + 11] = wk5r * x0i + wk5i * x0r;
		x0r = y3r - y7i;
		x0i = y3i + y7r;
		a[j + 6] = wk3r * x0r - wk3i * x0i;
		a[j + 7] = wk3r * x0i + wk3i * x0r;
		x0r = y3r + y7i;
		x0i = y3i - y7r;
		a[j + 14] = wk7r * x0r - wk7i * x0i;
		a[j + 15] = wk7r * x0i + wk7i * x0r;
		a[j] = y0r + y4r;
		a[j + 1] = y0i + y4i;
		x0r = y0r - y4r;
		x0i = y0i - y4i;
		a[j + 8] = wk4r * x0r - wk4i * x0i;
		a[j + 9] = wk4r * x0i + wk4i * x0r;
		x0r = y2r - y6i;
		x0i = y2i + y6r;
		a[j + 4] = wk2r * x0r - wk2i * x0i;
		a[j + 5] = wk2r * x0i + wk2i * x0r;
		x0r = y2r + y6i;
		x0i = y2i - y6r;
		a[j + 12] = wk6r * x0r - wk6i * x0i;
		a[j + 13] = wk6r * x0i + wk6i * x0r;
	}
	}
}


void cftmdl(int n, int l, float* a)
{
	int j, j1, j2, j3, j4, j5, j6, j7, k, kj, kr, m;
	float ew, wn4r, wtmp, wk1r, wk1i, wk2r, wk2i, wk3r, wk3i,
		wk4r, wk4i, wk5r, wk5i, wk6r, wk6i, wk7r, wk7i;
	float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
		y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
		y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i;

	m = l << 3;
	wn4r = WR5000;
cftmdl_label7:for (j = 0; j < l; j += 2) {
	j1 = j + l;
	j2 = j1 + l;
	j3 = j2 + l;
	j4 = j3 + l;
	j5 = j4 + l;
	j6 = j5 + l;
	j7 = j6 + l;
	x0r = a[j] + a[j1];
	x0i = a[j + 1] + a[j1 + 1];
	x1r = a[j] - a[j1];
	x1i = a[j + 1] - a[j1 + 1];
	x2r = a[j2] + a[j3];
	x2i = a[j2 + 1] + a[j3 + 1];
	x3r = a[j2] - a[j3];
	x3i = a[j2 + 1] - a[j3 + 1];
	y0r = x0r + x2r;
	y0i = x0i + x2i;
	y2r = x0r - x2r;
	y2i = x0i - x2i;
	y1r = x1r - x3i;
	y1i = x1i + x3r;
	y3r = x1r + x3i;
	y3i = x1i - x3r;
	x0r = a[j4] + a[j5];
	x0i = a[j4 + 1] + a[j5 + 1];
	x1r = a[j4] - a[j5];
	x1i = a[j4 + 1] - a[j5 + 1];
	x2r = a[j6] + a[j7];
	x2i = a[j6 + 1] + a[j7 + 1];
	x3r = a[j6] - a[j7];
	x3i = a[j6 + 1] - a[j7 + 1];
	y4r = x0r + x2r;
	y4i = x0i + x2i;
	y6r = x0r - x2r;
	y6i = x0i - x2i;
	x0r = x1r - x3i;
	x0i = x1i + x3r;
	x2r = x1r + x3i;
	x2i = x1i - x3r;
	y5r = wn4r * (x0r - x0i);
	y5i = wn4r * (x0r + x0i);
	y7r = wn4r * (x2r - x2i);
	y7i = wn4r * (x2r + x2i);
	a[j1] = y1r + y5r;
	a[j1 + 1] = y1i + y5i;
	a[j5] = y1r - y5r;
	a[j5 + 1] = y1i - y5i;
	a[j3] = y3r - y7i;
	a[j3 + 1] = y3i + y7r;
	a[j7] = y3r + y7i;
	a[j7 + 1] = y3i - y7r;
	a[j] = y0r + y4r;
	a[j + 1] = y0i + y4i;
	a[j4] = y0r - y4r;
	a[j4 + 1] = y0i - y4i;
	a[j2] = y2r - y6i;
	a[j2 + 1] = y2i + y6r;
	a[j6] = y2r + y6i;
	a[j6 + 1] = y2i - y6r;
}
}

void init(int target_h, int target_w, int scale_model_sz[4], float scale_factor[2]) {

	float scale_model_factor = 1;
	float init_target_area = target_h * target_w;
	size sz;

	sz.h = target_h * (1 + padding);
	sz.w = target_w * (1 + padding);

	if (init_target_area > scale_model_max_area)
		scale_model_factor = sqrt(scale_model_max_area / init_target_area);
	scale_model_sz[0] = floor(target_w * scale_model_factor);
	scale_model_sz[1] = floor(target_h * scale_model_factor);
	scale_model_sz[2] = target_w;
	scale_model_sz[3] = target_h;

	scale_factor[0] = powf(scale_step, ceil(log(5.0 / findmin(sz.h, sz.w)) / log(scale_step)));  //min
	scale_factor[1] = powf(scale_step, floor(log(findmin(imgH / target_h, imgW / target_w)) / log(scale_step)));  //max

}

short findmin(short x, short y) {
	short z;
	if (x>y)z = y;
	else z = x;

	return z;
}

//output xsf
void get_hog_scale(DATA_TYPE img[imgH * imgW], result res_in, int scale_model_sz[4], float currentScaleFactor, cmpx xsf[templong][nScales]) {
	int patch_sz[2] = { 0 };
	DATA_TYPE im_patch[imgH][imgW];
	DATA_TYPE im_patch_resized[modelsz][modelsz] = { 0 };
	float temp_cell = 0, out[templong][nScales] = { 0 };

	int i, j, a, x_end, y_end, x, y, ii, jj, gs, go, binx, biny, binz, idx, idy, addr, temp_int, gg, pos_x, pos_y, target_h, target_w;
	float tempy, tempx, temp, Iphase, temp1, temp2, temp3, temps, tt, temp0, temp01, temp02, temp03, temp4, temp04;
	DATA_TYPE Ied[modelsz][modelsz], gradorient[modelsz][modelsz];
	HOG_TYPE cell[featurelong][featurelong][nOrients - 1], hist3dbig[4][4][8] = { 0 };
	cmpx dft2_0[nScales], reg;
	size scale_model_temp;
	//double dft2_0r[nScales], dft2_0i[nScales], fdft2_0r[nScales], fdft2_0i[nScales];

	FILE* fp;

	pos_x = res_in.x;
	pos_y = res_in.y;
	target_h = res_in.height;
	target_w = res_in.width;

	int cell_size = 4, block = 8;
	int window_h = scale_model_sz[1];
	int window_w = scale_model_sz[0];
	scale_model_temp.w = (int)(scale_model_sz[0] / cell_size);
	scale_model_temp.h = (int)(scale_model_sz[1] / cell_size);

get_hog_scalelabel0:
	for (int t = 0; t < nScales; t++) {
		patch_sz[0] = floor(scale_model_sz[2] * currentScaleFactor * scaleFactors[t]);
		patch_sz[1] = floor(scale_model_sz[3] * currentScaleFactor * scaleFactors[t]);

		get_subwindowforscale(img, pos_x, pos_y, patch_sz, im_patch);
		bilinera_interpolation(im_patch, patch_sz, scale_model_sz, im_patch_resized);

		//通过卷积，求图像块的梯度矩阵Ied及梯度方向矩阵gradorient
	get_hog_scale_label1:
		for (i = 0; i < window_h; i++) {
		get_hog_scale_label2:
			for (j = 0; j < window_w; j++) {
				if (i == 0) {
					if (j == 0) {
						tempy = im_patch_resized[i][j + 1];
						temp = tempy * tempy;
					}
					else if (j == window_w - 1) {
						tempy = -im_patch_resized[i][j - 1];
						temp = tempy * tempy;
					}
					else {
						temp1 = im_patch_resized[i][j + 1];
						temp2 = im_patch_resized[i][j - 1];
						tempy = temp1 - temp2;
						temp = tempy * tempy;
					}
					temp1 = im_patch_resized[(i + 1)][j];
					temp2 = im_patch_resized[(i + 1)][j];
					tempx = temp1 * temp2;
					tt = tempx + temp;
					temps = sqrt(tt);
					Ied[i][j] = temps;
					temp_int = (int)temps;
					Ied[i][j] = temp_int;
					//	if (tt) Iphase = tempy / temps;
					if (tt) Iphase = tempy / temp_int;
					else Iphase = 0;
					temp = atanX(Iphase) + PI / 2;

					gradorient[i][j] = (int)temp;
					continue;
				}
				if (i == window_h - 1) {
					if (j == 0) {
						tempy = im_patch_resized[i][j + 1];
						temp = tempy * tempy;
					}
					else if (j == window_w - 1) {
						tempy = -im_patch_resized[i][j - 1];
						temp = tempy * tempy;
					}
					else {
						temp1 = im_patch_resized[i][j + 1];
						temp2 = im_patch_resized[i][j - 1];
						tempy = temp1 - temp2;
						temp = tempy * tempy;
					}
					tempx = im_patch_resized[(i - 1)][j] * im_patch_resized[(i - 1)][j];
					tt = tempx + temp;
					temps = sqrt(tt);
					Ied[i][j] = temps;
					temp_int = (int)temps;
					Ied[i][j] = temp_int;
					//if (tt) Iphase = tempy / temps;
					if (tt) Iphase = tempy / temp_int;
					else Iphase = 0;
					temp = atanX(Iphase) + PI / 2;

					gradorient[i][j] = (int)temp;
					continue;
				}
				if (j == 0) {
					tempy = im_patch_resized[i][j + 1];
					temp = tempy * tempy;
					temp1 = im_patch_resized[(i - 1)][j];
					temp2 = im_patch_resized[(i + 1)][j];
					tempx = temp1 - temp2;
					tempx = tempx * tempx;
					tt = tempx + temp;
					temps = sqrt(tt);
					Ied[i][j] = temps;
					temp_int = (int)temps;
					Ied[i][j] = temp_int;
					//if (tt) Iphase = tempy / temps;
					if (tt) Iphase = tempy / temp_int;
					else Iphase = 0;
					temp = atanX(Iphase) + PI / 2;

					gradorient[i][j] = (int)temp;
					continue;
				}
				if (j == window_w - 1) {
					tempy = -im_patch_resized[i][j - 1];
					temp = tempy * tempy;
					temp1 = im_patch_resized[(i - 1)][j];
					temp2 = im_patch_resized[(i + 1)][j];
					tempx = temp1 - temp2;
					tempx = tempx * tempx;
					tt = tempx + temp;
					temps = sqrt(tt);
					Ied[i][j] = temps;
					temp_int = (int)temps;
					Ied[i][j] = temp_int;
					//if (tt) Iphase = tempy / temps;
					if (tt) Iphase = tempy / temp_int;
					else Iphase = 0;
					temp = atanX(Iphase) + PI / 2;

					gradorient[i][j] = (int)temp;
					continue;
				}
				temp1 = im_patch_resized[i][j + 1];
				temp2 = im_patch_resized[i][j - 1];
				tempy = temp1 - temp2;
				temp = tempy * tempy;
				temp1 = im_patch_resized[(i - 1)][j];
				temp2 = im_patch_resized[(i + 1)][j];
				tempx = temp1 - temp2;
				tempx = tempx * tempx;
				tt = tempx + temp;
				temps = sqrt(tt);
				Ied[i][j] = temps;
				temp_int = (int)temps;
				Ied[i][j] = temp_int;
				//		if (tt) Iphase = tempy / temps;
				if (tt) Iphase = tempy / temp_int;
				else Iphase = 0;
				temp = atanX(Iphase) + PI / 2;

				gradorient[i][j] = (int)temp;
			}
		}
		x_end = window_w - block + 1;  //x方向块左角能达到的最大值
		y_end = window_h - block + 1;
		//计算hog特征
	get_hog_scale_label3:
		for (y = 0; y < y_end; y += block) {
		get_hog_scale_label4:
			for (x = 0; x < x_end; x += block) {
			get_hog_scale_label5:
				for (i = 0; i < block; i++) {
				get_hog_scale_label6:
					for (j = 0; j < block; j++) {
						ii = y + i;  //在整个坐标系中的坐标
						jj = x + j;
						gs = Ied[ii][jj];  //梯度值
						go = gradorient[ii][jj];  //梯度方向
												  // 计算八个统计区间中心点的坐标
						binx = (j + (cell_size >> 1)) / cell_size;
						biny = (i + (cell_size >> 1)) / cell_size;
						tempx = PI / nOrients;
						temp = go;
						temp = (temp + tempx * 0.5) / tempx;
						binz = (int)temp;
						if (gs == 0) continue;

						//梯度映射
						addr = hist3dbig[biny][binx][binz];
						hist3dbig[biny][binx][binz] = gs + addr;

						addr = hist3dbig[biny][binx][binz + 1];
						hist3dbig[biny][binx][binz + 1] = gs + addr;

						addr = hist3dbig[biny][binx + 1][binz];
						hist3dbig[biny][binx + 1][binz] = gs + addr;

						addr = hist3dbig[biny][binx + 1][binz + 1];
						hist3dbig[biny][binx + 1][binz + 1] = gs + addr;

						addr = hist3dbig[biny + 1][binx][binz];
						hist3dbig[biny + 1][binx][binz] = gs + addr;

						addr = hist3dbig[biny + 1][binx][binz + 1];
						hist3dbig[biny + 1][binx][binz + 1] = gs + addr;

						addr = hist3dbig[biny + 1][binx + 1][binz];
						hist3dbig[biny + 1][binx + 1][binz] = gs + addr;

						addr = hist3dbig[biny + 1][binx + 1][binz + 1];
						hist3dbig[biny + 1][binx + 1][binz + 1] = gs + addr;
					}
				}

				idx = x >> 3;
				idy = y >> 3;

				/*cell[idy << 1][idx << 1][4] = (im_patch_resized[y][x] + im_patch_resized[y + 1][x] + im_patch_resized[y + 1][x + 1] + im_patch_resized[y][x + 1]) >> 2;
				cell[(idy << 1) + 1][idx << 1][4] = (im_patch_resized[y + 2][x] + im_patch_resized[y + 3][x] + im_patch_resized[y + 3][x + 1] + im_patch_resized[y + 2][x + 1]) >> 2;
				cell[(idy << 1) + 1][(idx << 1) + 1][4] = (im_patch_resized[y + 2][x + 2] + im_patch_resized[y + 3][x + 2] + im_patch_resized[y + 3][x + 3] + im_patch_resized[y + 2][x + 3]) >> 2;
				cell[idy << 1][(idx << 1) + 1][4] = (im_patch_resized[y][x + 2] + im_patch_resized[y + 1][x + 2] + im_patch_resized[y + 1][x + 3] + im_patch_resized[y][x + 3]) >> 2;*/

			get_hog_scale_label7:
				for (i = 1; i < nOrients; i++) {  //舍去最后一维(全0)
					cell[idy << 1][idx << 1][i - 1] = hist3dbig[1][1][i];
					cell[(idy << 1) + 1][idx << 1][i - 1] = hist3dbig[2][1][i];
					cell[(idy << 1) + 1][(idx << 1) + 1][i - 1] = hist3dbig[2][2][i];
					cell[idy << 1][(idx << 1) + 1][i - 1] = hist3dbig[1][2][i];
				}
				memset(hist3dbig, 0, sizeof(hist3dbig));
			}
		}


		int z = 0;
		//把cell(hog featurea)变成一个列向量，按矩阵的列,一列一列变
	get_hog_scale_label8:
		for (int q = 0; q < nOrients - 4; q++) {
		get_hog_scale_label9:
			for (int i = 0; i < scale_model_temp.w; i++) {
			get_hog_scale_label10:
				for (int j = 0; j < scale_model_temp.h; j++) {
					temp_cell = cell[j][i][q];
					if (temp_cell < -80000) temp_cell = 0;
					out[z][t] = temp_cell * scale_window[t];
					z++;
				}
			}
		}

	}
	//对out做行fft	out[5*scale_model_temp.h*scale_model_temp.w][nScales] = dsst里的xs
get_hog_scale_label11:
	for (i = 0; i < (nOrients - 4) * scale_model_temp.h*scale_model_temp.w; i++) {
	get_hog_scale_label12:
		for (j = 0; j < nScales; j++) {
			reg.real = out[i][j];
			reg.imag = 0;
			dft2_0[j].real = reg.real;
			dft2_0[j].imag = reg.imag;
		}
		fftaa(nScales, dft2_0);
		//kfft(dft2_0r, dft2_0i, nScales, 5, fdft2_0r, fdft2_0i);
	get_hog_scale_label13:
		for (a = 0; a < nScales; a++) {
			reg.real = dft2_0[a].real;
			reg.imag = dft2_0[a].imag;
			xsf[i][a].imag = reg.imag;
			xsf[i][a].real = reg.real;
		}
	}
}

/*
* 输入：
* 		img 			150*300图像矩阵；
* 		patch_pos		目标中心点在图像矩阵中的坐标；
* 		SmallTarget		小目标标志符（高有效）；
* 输出：
* 		im_patch			截取的不固定大小子窗口
*/
void get_subwindowforscale(DATA_TYPE img[imgH * imgW], int pos_x, int pos_y, int patch_sz[2], DATA_TYPE im_patch[imgH][imgW]) {
	int i, j, x_low, y_low, ii, jj, ti, tj, window_h, window_w, tempi, tempj;
	DATA_TYPE temp;
	float l;

	window_h = patch_sz[1];
	window_w = patch_sz[0];

	//x,y方向上的起始位置
	y_low = pos_y - (window_h >> 1) + 1;
	x_low = pos_x - (window_w >> 1) + 1;

get_subwindowforscale_label0:
	for (i = 0; i < window_h; i++) {
	get_subwindowforscale_label1:
		for (j = 0; j < window_w; j++) {
			tempi = y_low + i;
			tempj = x_low + j;

			if (tempi < 0) ii = 0;
			else if (tempi > imgH - 1) ii = imgH - 1; //如果目标在边缘，超出图像范围，则重复取边缘
			else ii = tempi;

			if (tempj < 0) jj = 0;
			else if (tempj > imgW - 1) jj = imgW - 1;
			else jj = tempj;

			ti = imgW * ii;
			tj = ti + jj;
			l = img[tj];
			//l = sqrt(l);
			temp = round(l);
			im_patch[i][j] = temp;
		}
	}
}

//output im_patch_resized
void bilinera_interpolation(DATA_TYPE im_patch[imgH][imgW], int patch_sz[2], int scale_model_sz[4], DATA_TYPE im_patch_resized[modelsz][modelsz])
{
	double out_height = scale_model_sz[1];   //resized 图片大小
	double out_width = scale_model_sz[0];
	int height = patch_sz[1];               //原图大小
	int width = patch_sz[0];
	double h_times = (double)out_height / (double)height,
		w_times = (double)out_width / (double)width;
	short x1, y1, x2, y2, f11, f12, f21, f22;
	double x, y;
	int i, j;
	DATA_TYPE im_patch2[imgH][imgW] = { 0 };

bilinera_interpolation_label0:
	for (i = 2; i < height + 2; i++) {                        //扩左右两列
		im_patch2[i][1] = im_patch[i - 2][0];
		im_patch2[i][width + 2] = im_patch[i - 2][width - 1];
	}

bilinera_interpolation_label1:
	for (j = 2; j < width + 2; j++) {                       //扩上下两行
		im_patch2[1][j] = im_patch[0][j - 2];
		im_patch2[height + 2][j] = im_patch[height - 1][j - 2];
	}
	im_patch2[1][1] = im_patch[0][0];                                     //左上
	im_patch2[1][width + 2] = im_patch[0][width - 1];                     //右上
	im_patch2[height + 2][1] = im_patch[height - 1][0];                   //左下
	im_patch2[height + 2][width + 2] = im_patch[height - 1][width - 1];   //右下
	
bilinera_interpolation_label2:
	for (i = 2; i < height + 2; i++) {
	bilinera_interpolation_label3:
		for (j = 2; j < width + 2; j++) {
			im_patch2[i][j] = im_patch[i - 2][j - 2];
		}
	}

bilinera_interpolation_label4:
	for (i = 1; i < out_height + 1; i++) {
	bilinera_interpolation_label5:
		for (j = 1; j < out_width + 1; j++) {
			x = (j + 0.5) / w_times - 0.5;
			y = (i + 0.5) / h_times - 0.5;
			x1 = ceil(x - 1);
			if (x1 < 1)
				x1 = 1;

			x2 = ceil(x + 1);
			if (x2 >(width + 2))
				x2 = width + 2;

			y1 = ceil(y + 1);
			if (y1 > (height + 2))
				y1 = height + 2;

			y2 = ceil(y - 1);
			if (y2 < 1)
				y2 = 1;

			f11 = im_patch2[y1][x1];
			f12 = im_patch2[y2][x1];
			f21 = im_patch2[y1][x2];
			f22 = im_patch2[y2][x2];
			im_patch_resized[i - 1][j - 1] = floor(((f11 * (x2 - x) * (y2 - y)) +
				(f21 * (x - x1) * (y2 - y)) +
				(f12 * (x2 - x) * (y - y1)) +
				(f22 * (x - x1) * (y - y1))) / ((x2 - x1) * (y2 - y1)));
		}
	}
	
}


//new_sf_num = bsxfun(@times, ysf, conj(xsf))
//new_sf_den = sum(xsf .* conj(xsf), 1) 把矩阵按列相加
void creatnew_sf_num_den(int scale_model_sz[4], cmpx xsf[templong][nScales], cmpx sf_num[templong][nScales], float sf_den[nScales], bool frame) {
	size scale_model_temp;
	int i, j, cell_size = 4;
	float temp = 0;
	cmpx new_sf_num[templong][nScales];
	float new_sf_den[nScales];

	scale_model_temp.w = (int)(scale_model_sz[0] / cell_size);
	scale_model_temp.h = (int)(scale_model_sz[1] / cell_size);

creatnew_sf_num_den_label0:
	for (i = 0; i < (nOrients - 4) * scale_model_temp.w*scale_model_temp.h; i++) {
	creatnew_sf_num_den_label1:
		for (j = 0; j < nScales; j++) {
			new_sf_num[i][j].real = ysf[2 * j] * xsf[i][j].real + ysf[2 * j + 1] * xsf[i][j].imag;
			new_sf_num[i][j].imag = ysf[2 * j + 1] * xsf[i][j].real - ysf[2 * j] * xsf[i][j].imag;

		}
	}

creatnew_sf_num_den_label2:
	for (i = 0; i < nScales; i++) {
	creatnew_sf_num_den_label3:
		for (j = 0; j < (nOrients - 4) * scale_model_temp.w*scale_model_temp.h; j++) {

			temp = temp + (int)xsf[j][i].real * (int)xsf[j][i].real + (int)xsf[j][i].imag * (int)xsf[j][i].imag;
		}
		new_sf_den[i] = temp;
		temp = 0;
	}

	if(frame){

	KCF_tracker_label0:
		for (i = 0; i < nScales; i++) {
					sf_den[i] = new_sf_den[i];
		}
	KCF_tracker_label1:
		for (i = 0; i < (nOrients - 4) * scale_model_temp.w*scale_model_temp.h; i++) {
		KCF_tracker_label2:
			for (j = 0; j < nScales; j++) {
				sf_num[i][j].real = new_sf_num[i][j].real;
				sf_num[i][j].imag = new_sf_num[i][j].imag;
			}
		}
	}
	else{

	update_label0:
		for (i = 0; i < nScales; i++) {
			sf_den[i] = (1 - learning_rate) * sf_den[i] + learning_rate * new_sf_den[i];
		}

	update_label1:
		for (i = 0; i < (nOrients - 4) * scale_model_temp.w*scale_model_temp.h; i++) {
		update_label2:
			for (j = 0; j < nScales; j++) {
			    sf_num[i][j].real = (1 - learning_rate) * sf_num[i][j].real + learning_rate * new_sf_num[i][j].real;
			    sf_num[i][j].imag = (1 - learning_rate) * sf_num[i][j].imag + learning_rate * new_sf_num[i][j].imag;
			}
		}
	}
}

short get_response_scale(int scale_model_sz[4], cmpx sf_num[templong][nScales], cmpx xsf[templong][nScales], float sf_den[nScales]) {
	int i, j, k, cell_size = 4;
	float max;
	float scale_response[nScales];
	cmpx dft[nScales], temp = { 0 };
	size scale_model_temp;
	//double dftr[nScales], dfti[nScales], fdftr[nScales], fdfti[nScales];

	scale_model_temp.w = (int)(scale_model_sz[0] / cell_size);
	scale_model_temp.h = (int)(scale_model_sz[1] / cell_size);

	//sum(sf_num .* xsf, 1).real
get_response_scale_label0:
	for (i = 0; i < nScales; i++) {
	get_response_scale_label1:
		for (j = 0; j < (nOrients - 4) * scale_model_temp.w*scale_model_temp.h; j++) {
			temp.real = temp.real + sf_num[j][i].real * xsf[j][i].real - sf_num[j][i].imag * xsf[j][i].imag;
			temp.imag = temp.imag + sf_num[j][i].real * xsf[j][i].imag + sf_num[j][i].imag * xsf[j][i].real;
		}
		dft[i].real = temp.real / (sf_den[i] + lambda);
		dft[i].imag = temp.imag / (sf_den[i] + lambda);
		temp.real = 0;
		temp.imag = 0;
	}

	ifftaa(nScales, dft);  //ifft
	//kkfft(dftr, dfti, nScales, 5, fdftr, fdfti, 1, 0)

get_response_scale_label2:
	for (k = 0; k < nScales; k++) {
		scale_response[k] = dft[k].real;
	}

	//找到响应最大值位置，即为目标位移
	max = scale_response[0];
	short int cnt = 0;

get_response_scale_label3:
	for (i = 1; i < nScales; i++) {
		if (scale_response[i] > max) {
			max = scale_response[i];
			cnt = i;
		}
	}

	return cnt;
}


void conjugate_cmpx(int n, cmpx in[], cmpx out[])
{
	int i = 0;
conjugate_cmpx_label0:
	for (i = 0; i<n; i++)
	{
		out[i].imag = -in[i].imag;
		out[i].real = in[i].real;
	}
}


//傅里叶变化
void fftaa(int N, cmpx f[])
{
	cmpx t, wn;//中间变量
	int i, j, k, m, n, l, r, M;
	int la, lb, lc;
	/*----计算分解的级数M=log2(N)----*/
fftaa_label0:
	for (i = N, M = 1; (i = i / 2) != 1; M++);
	/*----按照倒位序重新排列原信号----*/
fftaa_label1:
	for (i = 1, j = N / 2; i <= N - 2; i++)
	{
		if (i<j)
		{
			t = f[j];
			f[j] = f[i];
			f[i] = t;
		}
		k = N / 2;
	fftaa_label1_1:
		while (k <= j)
		{
			j = j - k;
			k = k / 2;
		}
		j = j + k;
	}

	/*----FFT算法----*/
fftaa_label2:
	for (m = 1; m <= M; m++)
	{
		la = pow(2, m); //la=2^m代表第m级每个分组所含节点数		
		lb = la / 2;    //lb代表第m级每个分组所含碟形单元数
						//同时它也表示每个碟形单元上下节点之间的距离
						/*----碟形运算----*/
	fftaa_label3:
		for (l = 1; l <= lb; l++)
		{
			r = (l - 1)*pow(2, M - m);
		fftaa_label4:
			for (n = l - 1; n<N - 1; n = n + la) //遍历每个分组，分组总数为N/la
			{
				lc = n + lb;  //n,lc分别代表一个碟形单元的上、下节点编号     
				wn.real = sin_cosX(2 * PI*r / N, 1);
				wn.imag = -sin_cosX(2 * PI*r / N, 0);

				t.real = f[lc].real * wn.real - f[lc].imag * wn.imag;
				t.imag = f[lc].real * wn.imag + f[lc].imag * wn.real;

				f[lc].real = f[n].real - t.real;
				f[lc].imag = f[n].imag - t.imag;

				f[n].real = f[n].real + t.real;
				f[n].imag = f[n].imag + t.imag;

				//c_mul(f[lc], wn, &t);//t = f[lc] * wn复数运算
				//c_sub(f[n], t, &(f[lc]));//f[lc] = f[n] - f[lc] * Wnr
				//c_plus(f[n], t, &(f[n]));//f[n] = f[n] + f[lc] * Wnr
			}
		}
	}
}

//傅里叶逆变换
void ifftaa(int N, cmpx f[])
{
	int i = 0;
	conjugate_cmpx(N, f, f);
	fftaa(N, f);
	conjugate_cmpx(N, f, f);

ifftaa_label2:
	for (i = 0; i<N; i++)
	{
		f[i].imag = (f[i].imag) / N;
		f[i].real = (f[i].real) / N;
	}
}
