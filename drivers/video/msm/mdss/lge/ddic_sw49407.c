#define pr_fmt(fmt)	"[Display] %s: " fmt, __func__

#include <linux/delay.h>
#include "../mdss_dsi.h"

#define NUM_SHA_CTRL 		5
#define NUM_SAT_CTRL 		5
#define OFFSET_SAT_CTRL 	5
#define NUM_HUE_CTRL 		5

#define SC_MODE_MAX 		4
#define NUM_DG_PRESET       5

#define DG_MASK_ELSA 0x08

static char sha_ctrl_values[NUM_SHA_CTRL] = {0x04, 0x0D, 0x1A, 0x30, 0xD2};  //sharpness

static char sat_ctrl_values[NUM_SAT_CTRL][OFFSET_SAT_CTRL] = {               //saturation
	{0x00, 0x38, 0x70, 0xA8, 0xE1},
	{0x00, 0x3C, 0x78, 0xB4, 0xF1},
	{0x00, 0x43, 0x83, 0xC0, 0xFF},
	{0x00, 0x43, 0x87, 0xCB, 0xFF},
	{0x00, 0x47, 0x8F, 0xD7, 0xFF}
};

static char hue_ctrl_values[NUM_HUE_CTRL] = {0xF7, 0xF4, 0xF0, 0xE4, 0xE7};

static char white_mode_value[NUM_DG_PRESET][OFFSET_DG_CTRL] = {                //temperature value
	{0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40},
	{0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F},
	{0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E},
	{0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D},
	{0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C}
};

extern void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
		struct dsi_panel_cmds *pcmds, u32 flags);

static void lge_set_white_mode_send_sw49407(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int i = 0;
	int rgb_index = 0;
	char *payload_ctrl[3] = {NULL, };

	struct dsi_panel_cmds *pcmds;

	if (ctrl == NULL) {
		pr_err("Invalid input\n");
		return;
	}

	pcmds = lge_get_extra_cmds_by_name(ctrl, "white-dummy");
	if (pcmds) {
		for (i = 0; i < pcmds->cmd_cnt; i++)
			payload_ctrl[i] = pcmds->cmds[i].payload;
	} else {
		pr_err("no cmds: white-dummy\n");
		return;
	}

	rgb_index = ctrl->lge_extra.white_mode;
	pr_info("rgb_index=(%d)\n", rgb_index);

	if(rgb_index == 0)
		ctrl->lge_extra.dg_control = DG_OFF;
	else
		ctrl->lge_extra.dg_control = DG_ON;

	for (i = 1; i < OFFSET_DG_CTRL+1; i++) {
		payload_ctrl[RED][i] = white_mode_value[rgb_index][i-1];
		payload_ctrl[GREEN][i] = white_mode_value[rgb_index][i-1];
		payload_ctrl[BLUE][i] = white_mode_value[rgb_index][i-1];
	}
	mdss_dsi_panel_cmds_send(ctrl, pcmds, CMD_REQ_COMMIT);

	pr_info("[0x%02x:0x%02x][0x%02x:0x%02x][0x%02x:0x%02x]\n",
			payload_ctrl[RED][0], payload_ctrl[RED][1],
			payload_ctrl[GREEN][0], payload_ctrl[GREEN][1],
			payload_ctrl[BLUE][0], payload_ctrl[BLUE][1]);

	return;
}

static void lge_set_screen_tune_send_sw49407(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int i = 0;
	char *payload_ctrl[2] = {NULL, };
	struct dsi_panel_cmds *pcmds;

	pcmds = lge_get_extra_cmds_by_name(ctrl, "color-dummy");
	if (pcmds) {
		for (i = 0; i < pcmds->cmd_cnt; i++)
			payload_ctrl[i] = pcmds->cmds[i].payload;
	} else {
		pr_err("no cmds: color-dummy\n");
		return;
	}

	if (ctrl->lge_extra.sc_sat_step > SC_MODE_MAX)
		ctrl->lge_extra.sc_sat_step = SC_MODE_MAX;
	if (ctrl->lge_extra.sc_hue_step > SC_MODE_MAX)
		ctrl->lge_extra.sc_hue_step = SC_MODE_MAX;
	if (ctrl->lge_extra.sc_sha_step > SC_MODE_MAX)
		ctrl->lge_extra.sc_sha_step = SC_MODE_MAX;

	payload_ctrl[0][3] = sha_ctrl_values[ctrl->lge_extra.sc_sha_step];             //sharpness
	if(ctrl->lge_extra.sc_sha_step != SHARP_DEFAULT)
		ctrl->lge_extra.sharpness_control = true;
	else
		ctrl->lge_extra.sharpness_control = false;

	for (i = 0; i < OFFSET_SAT_CTRL; i++)
		payload_ctrl[1][i+1] = sat_ctrl_values[ctrl->lge_extra.sc_sat_step][i];    //saturation
	
	payload_ctrl[1][6] = hue_ctrl_values[ctrl->lge_extra.sc_hue_step];    //hue
	
	mdss_dsi_panel_cmds_send(ctrl, pcmds, CMD_REQ_COMMIT);
	pr_info("sat : [0x%02x:0x%02x:0x%02x]  hue: [0x%02x] sharp : [0x%02x]\n",
			payload_ctrl[1][1], payload_ctrl[1][2], payload_ctrl[1][3],
			payload_ctrl[1][6], payload_ctrl[0][3]);
	return;
}


void sharpness_set_send_sw49407(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int mode = 0;
	struct dsi_panel_cmds *pcmds;

	if (ctrl->lge_extra.sharpness_control == true ) {
		pr_info("skip sharpness control! \n");
		return;
	}

	pcmds = lge_get_extra_cmds_by_name(ctrl, "sharpness");
	mode = ctrl->lge_extra.sharpness;

	if (pcmds) {
		pcmds->cmds[0].payload[3] = mode;
		pr_info("sharpness=%d\n", mode);
		mdss_dsi_panel_cmds_send(ctrl, pcmds, CMD_REQ_COMMIT);
	} else {
		pr_err("no cmds: sharpness\n");
	}
}

void lge_set_image_quality_cmds(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char mask = 0x00;
	struct dsi_panel_cmds *ie_pcmds;			//55h
	struct dsi_panel_cmds *mie_pcmds;			//f0h

	ie_pcmds = lge_get_extra_cmds_by_name(ctrl, "ie-ctrl");
	if (!ie_pcmds)
	{
		pr_err("no cmds: ie-ctrl\n");
		return;
	}

	mie_pcmds = lge_get_extra_cmds_by_name(ctrl, "mie-ctrl");
	if (!mie_pcmds)
	{
		pr_err("no cmds: mie-ctrl\n");
		return;
	}

#if defined(CONFIG_LGE_LCD_DYNAMIC_CABC_MIE_CTRL)
	if (ctrl->lge_extra.ie_control == 0) {
		pr_info("%s: IE OFF\n",__func__);
		mask = IE_MASK;
		ie_pcmds->cmds[0].payload[1] &= (~mask);
		goto send;
	}
#endif

	//start mie_pcmds(F0h) setting
	mask = (SH_MASK | SAT_MASK | HUE_MASK | DG_MASK_ELSA);
	mie_pcmds->cmds[0].payload[1] &= (~mask);  //F0h

	if(ctrl->lge_extra.dg_control == DG_ON)
		mie_pcmds->cmds[0].payload[1] |= DG_MASK_ELSA;

	mie_pcmds->cmds[0].payload[1] |= SH_MASK | SAT_MASK | HUE_MASK;
	//end mie_pcmds(F0h) setting

send:
    mdss_dsi_panel_cmds_send(ctrl, ie_pcmds, CMD_REQ_COMMIT);
	mdss_dsi_panel_cmds_send(ctrl, mie_pcmds, CMD_REQ_COMMIT);
	pr_info("%s : 55h:0x%02x, f0h:0x%02x \n",__func__,
			ie_pcmds->cmds[0].payload[1],  mie_pcmds->cmds[0].payload[1]);
}

void dic_lcd_mode_set(struct mdss_dsi_ctrl_pdata *ctrl)
{
	if (ctrl == NULL) {
		pr_err("%pS: ctrl == NULL\n", __builtin_return_address(0));
		return;
	}

	if (mdss_dsi_is_panel_off(&ctrl->panel_data) || mdss_dsi_is_panel_on_lp(&ctrl->panel_data)) {
		pr_info("unexpected panel power state while setting lcd mode\n");
		return;
	}

	lge_set_white_mode_send_sw49407(ctrl);
	lge_set_screen_tune_send_sw49407(ctrl);
	sharpness_set_send_sw49407(ctrl);
	lge_set_image_quality_cmds(ctrl);
}

static int image_enhance_set_sw49407(struct mdss_dsi_ctrl_pdata *ctrl, int mode)
{
	struct dsi_panel_cmds *ie_pcmds;

	if (ctrl == NULL)
		return -ENODEV;

	ie_pcmds = lge_get_extra_cmds_by_name(ctrl, "ie-ctrl");
	if (!ie_pcmds)
	{
		pr_err("no cmds: ie-ctrl\n");
		return -EINVAL;
	}

	ctrl->lge_extra.ie_control = mode;
	pr_info("ie_control : %d\n", mode);

	if (ctrl->lge_extra.ie_control == 1) {
		ie_pcmds->cmds[0].payload[1] |= IE_MASK;
	} else if (ctrl->lge_extra.ie_control == 0) {
		char mask = IE_MASK;
		ie_pcmds->cmds[0].payload[1] &= (~mask);
	} else {
		pr_info("%s: set = %d, wrong set value\n", __func__, ctrl->lge_extra.ie_control);
	}

	mdss_dsi_panel_cmds_send(ctrl, ie_pcmds, CMD_REQ_COMMIT);
	pr_info("%s : 55h:0x%02x\n",__func__,ie_pcmds->cmds[0].payload[1]);
	return 0;
}

static int image_enhance_get_sw49407(struct mdss_dsi_ctrl_pdata *ctrl)
{
	if (ctrl == NULL)
		return -ENODEV;
	return ctrl->lge_extra.ie_control;
}

static int white_mode_set_sw49407(struct mdss_dsi_ctrl_pdata *ctrl, int mode)
{
	if (ctrl == NULL)
		return -ENODEV;
	ctrl->lge_extra.white_mode = mode;

	pr_info("white mode : %d\n", ctrl->lge_extra.white_mode);

	dic_lcd_mode_set(ctrl);
	return 0;
}

static int screen_tune_set_sw49407(struct mdss_dsi_ctrl_pdata *ctrl)
{
	if (ctrl == NULL)
		return -ENODEV;

	pr_info("sat : %d , hue = %d , sha = %d \n",
			ctrl->lge_extra.sc_sat_step, ctrl->lge_extra.sc_hue_step,
			ctrl->lge_extra.sc_sha_step);

	dic_lcd_mode_set(ctrl);
	return 0;
}

static int sharpness_set_sw49407(struct mdss_dsi_ctrl_pdata *ctrl, int mode)
{
	if (ctrl == NULL)
		return -ENODEV;

	ctrl->lge_extra.sharpness = mode;
	sharpness_set_send_sw49407(ctrl);
	return 0;
}

static struct lge_ddic_ops sw49407_ops = {
	.op_image_enhance_set = image_enhance_set_sw49407,
	.op_image_enhance_get = image_enhance_get_sw49407,

	.op_white_mode_set = white_mode_set_sw49407,
	.op_screen_tune_set  = screen_tune_set_sw49407,
	.op_sharpness_set  = sharpness_set_sw49407,
};

struct lge_ddic_ops *get_ddic_ops_sw49407(void)
{
	return &sw49407_ops;
}
