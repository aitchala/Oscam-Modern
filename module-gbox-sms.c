#include "globals.h"

#ifdef MODULE_GBOX
#include "module-gbox.h"
#include "module-gbox-sms.h"
#include "oscam-string.h"
#include "oscam-files.h"
#include "oscam-string.h"
#include "oscam-client.h"
#include "oscam-time.h"

static uint32_t poll_gsms_data (uint16_t *boxid, uint8_t *num, char *text)
{
	FILE *fhandle = fopen(FILE_GSMS_TXT, "r");
	if(!fhandle)
	{
		cs_log("Couldn't open %s: %s", FILE_GSMS_TXT, strerror(errno));
		return -1;
	}
	uint32_t length1;
	uint8_t length;
	char buffer[140];
	memset(buffer, 0, sizeof(buffer));
	fseek (fhandle,0L,SEEK_END);
	length1 = ftell(fhandle);
	fseek (fhandle,0L,SEEK_SET);
	if (length1 < 12)
	{
		cs_log("GSMS: min msg char in %s = 5, actual = %d",FILE_GSMS_TXT, length1-7);
		fclose(fhandle);
		unlink(FILE_GSMS_TXT);
		return -1;
	}
	fgets(buffer,140,fhandle);
	fclose(fhandle);
	unlink(FILE_GSMS_TXT);
	char *tail;
	*boxid = strtol (buffer, &tail, 16);
	*num = atoi (tail);
	//*num = atoi ( &(buffer[5]));
	if (length1 > (127+7))
	{
		length = 127+7;
	}
	else
	{
	length = length1;
	}
	cs_debug_mask(D_READER, "GSMS: total msg length taken from %s = %d, limitted to %d",FILE_GSMS_TXT,length1, length);
	strncpy(text, &(buffer[7]),length-7);
	return 0;
}

static void write_gsms_to_osd_file(struct s_client *cli, unsigned char *gsms)
{
	if (file_exists(FILE_OSD_MSG))
	{
	char gsms_buf[150];
	memset(gsms_buf, 0, sizeof(gsms_buf));
	snprintf(gsms_buf, sizeof(gsms_buf), "%s %s:%s %s", FILE_OSD_MSG, username(cli), cli->reader->device, gsms);
	cs_debug_mask(D_READER, "GSMS: found OSD 'driver' %s - write gsms to OSD", FILE_OSD_MSG);
	char *cmd = gsms_buf;
              FILE *p;
              if ((p = popen(cmd, "w")) == NULL)
		{	
		cs_log("Error %s",FILE_OSD_MSG);
		return;
		}
              pclose(p);
	}
	return;
}

void write_gsms_ack (struct s_client *cli, uint8_t gsms_prot)
{
	char tsbuf[28];
	time_t walltime = cs_time();
	cs_ctime_r(&walltime, tsbuf);
	struct gbox_peer *peer = cli->gbox;

	FILE *fhandle = fopen(FILE_GSMS_ACK, "a+");
	if(!fhandle)
	{
		cs_log("Couldn't open %s: %s", FILE_GSMS_ACK, strerror(errno));
		return;
	}
	fprintf(fhandle, "Peer %04X (%s) confirmed receipt of GSMS_%d on %s",peer->gbox.id, cli->reader->device, gsms_prot, tsbuf);
	fclose(fhandle);
	return;
}

static void write_gsms_nack (struct s_client *cl, uint8_t gsms_prot, uint8_t inf)
{
	char tsbuf[28];
	time_t walltime = cs_time();
	cs_ctime_r(&walltime, tsbuf);
	struct gbox_peer *peer = cl->gbox;

	FILE *fhandle = fopen(FILE_GSMS_NACK, "a+");
	if(!fhandle)
	{
		cs_log("Couldn't open %s: %s", FILE_GSMS_NACK, strerror(errno));
		return;
	}
	if(inf)
	{
	fprintf(fhandle, "INFO: GSMS_%d to all: Peer %04X (%s) was OFFLINE %s",gsms_prot,peer->gbox.id, cl->reader->device,tsbuf);
	}
	else
	{
	fprintf(fhandle, "WARNING: GSMS_%d private to Peer %04X (%s) failed - was OFFLINE %s",gsms_prot,peer->gbox.id, cl->reader->device,tsbuf);
	}
	fclose(fhandle);
	return;
}

void write_gsms_msg (struct s_client *cli, uchar *gsms, uint16_t type, uint16_t UNUSED(msglen))
{
	char tsbuf[28];
	time_t walltime = cs_time();
	cs_ctime_r(&walltime, tsbuf);
	struct gbox_peer *peer = cli->gbox;
	struct s_reader *rdr = cli->reader;

	FILE *fhandle = fopen(FILE_GSMS_MSG, "a+");
	if(!fhandle)
	{
		cs_log("Couldn't open %s: %s", FILE_GSMS_MSG, strerror(errno));
		return;
	}
	if(type == 0x30)
		{
		fprintf(fhandle, "Normal message received from %04X %s on %s%s\n\n",peer->gbox.id, cli->reader->device, tsbuf, gsms);
		snprintf(rdr->last_gsms, sizeof(rdr->last_gsms), "%s %s", gsms, tsbuf); //added for easy handling of gsms by webif
		}
	else if(type == 0x31)
		{
		fprintf(fhandle, "OSD message received from %04X %s on %s%s\n\n",peer->gbox.id, cli->reader->device, tsbuf, gsms);
		write_gsms_to_osd_file(cli, gsms);
		snprintf(rdr->last_gsms, sizeof(rdr->last_gsms), "%s %s", gsms, tsbuf); //added for easy handling of gsms by webif
		}
	else 
		{fprintf(fhandle, "Corrupted message received from %04X %s on %s%s\n\n",peer->gbox.id, cli->reader->device, tsbuf, gsms);}
		fclose(fhandle);
	return;
}

void gsms_unavail(void)
{
	cs_log("INFO: GSMS feature disabled by conf");
}

static void gbox_send_gsms2peer(struct s_client *cl, char *gsms, uint8_t msg_type, uint8_t gsms_prot, int8_t gsms_len)
{
	uchar outbuf[150];
	struct gbox_peer *peer = cl->gbox;
	uint16_t local_gbox_id = gbox_get_local_gbox_id();
	uint32_t local_gbox_pw = gbox_get_local_gbox_password();
	struct s_reader *rdr = cl->reader;

			if (gsms_prot == 1)
			{
				gbox_code_cmd(outbuf, MSG_GSMS_1);
				outbuf[2] = gsms_len; // gsms len 
				outbuf[3] = msg_type;  //msg type
				memcpy(&outbuf[4], gsms,gsms_len);
				cs_log("<-[gbx] send GSMS_1 to %s:%d id: %04X", rdr->device, rdr->r_port, peer->gbox.id);
				gbox_send(cl, outbuf, gsms_len + 4);
			}
			if (gsms_prot == 2)
			{
				gbox_code_cmd(outbuf, MSG_GSMS_2);
				memcpy(outbuf + 2, peer->gbox.password, 4);
				outbuf[6] = (local_gbox_pw >> 24) & 0xff;
				outbuf[7] = (local_gbox_pw >> 16) & 0xff;
				outbuf[8] = (local_gbox_pw >> 8) & 0xff;
				outbuf[9] = local_gbox_pw & 0xff;				
				outbuf[10] = (peer->gbox.id >> 8) & 0xff;
				outbuf[11] = peer->gbox.id & 0xff;
				outbuf[12] = (local_gbox_id >> 8) & 0xff;
				outbuf[13] = local_gbox_id & 0xff;								
				outbuf[14] = msg_type; //msg type
				outbuf[15] = gsms_len; // gsms length
				memcpy(&outbuf[16], gsms,gsms_len);
				outbuf[16 + gsms_len] = 0; //last byte 0x00
				cs_log("<-[gbx] send GSMS_2 to %s:%d id: %04X", rdr->device, rdr->r_port, peer->gbox.id);
				gbox_send(cl, outbuf, gsms_len + 17);
			}
	return;
}

void gbox_init_send_gsms(void)
{
	uint16_t boxid = 0;
	uint8_t num = 0;
	uint8_t gsms_prot = 0;
	uint8_t msg_type = 0;
	char text[150];
	memset(text, 0, sizeof(text));

	if(cfg.gsms_dis)
	{
	unlink(FILE_GSMS_TXT);
	gsms_unavail();
	return;
	}
	if (poll_gsms_data( &boxid, &num, text))
	{
	cs_log("GSMS: ERROR polling file %s", FILE_GSMS_TXT);
	return;
	}
	int8_t gsms_len = strlen(text);
	cs_debug_mask(D_READER,"GSMS: got from %s: box_ID = %04X  num = %d  gsms_length = %d  txt = %s",FILE_GSMS_TXT, boxid, num, gsms_len, text);

	switch(num)
	{
	case 0: {gsms_prot = 1; msg_type = 0x30; break;}
	case 1: {gsms_prot = 1; msg_type = 0x31; break;}
	case 2: {gsms_prot = 2;	msg_type = 0x30; break;}
	case 3: {gsms_prot = 2;	msg_type = 0x31; break;}
	default:{cs_log("GSMS: ERROR unknown gsms protocol"); return;}
	}
	cs_debug_mask(D_READER,"init GSMS: gsms_length=%d  msg_type=%02X msg_prot=%d",gsms_len, msg_type, gsms_prot);

	struct s_client *cl;
	for (cl = first_client; cl; cl = cl->next)
	{
		if(cl->gbox && cl->typ == 'p')
		{

			struct gbox_peer *peer = cl->gbox;
			if (peer->online && boxid == 0xFFFF) //send gsms to all peers online
			{
			gbox_send_gsms2peer(cl, text, msg_type, gsms_prot, gsms_len); 
			}
			if (!peer->online && boxid == 0xFFFF)
			{
			cs_log("Info: peer %04X is OFFLINE",peer->gbox.id); 
			write_gsms_nack( cl, gsms_prot, 1); 
			}
			if (peer->online && boxid == peer->gbox.id)
			{
			gbox_send_gsms2peer(cl, text, msg_type, gsms_prot, gsms_len); 
			}
			if (!peer->online && boxid == peer->gbox.id)
			{
			cs_log("WARNING: send GSMS failed - peer %04X is OFFLINE",peer->gbox.id);
			write_gsms_nack( cl, gsms_prot, 0);  
			}
		}
	}
	return;
}

void gbox_send_gsms_ack(struct s_client *cli, uint8_t gsms_prot)
{
	uchar outbuf[20];
	struct gbox_peer *peer = cli->gbox;
	uint16_t local_gbox_id = gbox_get_local_gbox_id();
	uint32_t local_gbox_pw = gbox_get_local_gbox_password();
	struct s_reader *rdr = cli->reader;
		if (peer->online && gsms_prot == 1)
		{
		gbox_code_cmd(outbuf, MSG_GSMS_ACK_1);
		outbuf[2] = 0x90;
		outbuf[3] = 0x98; 
		outbuf[4] = 0x90;
		outbuf[5] = 0x98;
		outbuf[6] = 0x90;
		outbuf[7] = 0x98;
		outbuf[8] = 0x90;
		outbuf[9] = 0x98; 
		gbox_send(cli, outbuf, 10);
		cs_debug_mask(D_READER,"<-[gbx] send GSMS_ACK_1 to %s:%d id: %04X",rdr->device, rdr->r_port, peer->gbox.id);
		}
		if (peer->online && gsms_prot == 2)
		{
		gbox_code_cmd(outbuf, MSG_GSMS_ACK_2);
		memcpy(outbuf + 2, peer->gbox.password, 4);
		outbuf[6] = (local_gbox_pw >> 24) & 0xff;
		outbuf[7] = (local_gbox_pw >> 16) & 0xff;
		outbuf[8] = (local_gbox_pw >> 8) & 0xff;
		outbuf[9] = local_gbox_pw & 0xff;						
		outbuf[10] = 0;
		outbuf[11] = 0;
		outbuf[12] = (local_gbox_id >> 8) & 0xff;
		outbuf[13] = local_gbox_id & 0xff;									
		outbuf[14] = 0x1;
		outbuf[15] = 0;
		cs_debug_mask(D_READER,"<-[gbx] send GSMS_ACK_2 to %s:%d id: %04X",rdr->device, rdr->r_port, peer->gbox.id);
		gbox_send(cli, outbuf, 16);
		}
}
#endif
