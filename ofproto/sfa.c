/*
 * 	Auhtor : eric
 * 	Description : sfa module
 */


#include "sfa.h"
#include "connmgr.h"

#include "util.h"
#include "openvswitch/vlog.h"
#include "stdlib.h"
#include "string.h"

VLOG_DEFINE_THIS_MODULE(sfa);

//#ifdef __DEBUG__
//#define printf(STR) printf(STR)
//#endif


/*
 * Description		called in sfa_msg_handle procedure, the function will process the sfa init msg
 * 					it will init the status table, status transition table, action table. we now define
 * 					one status table for one application. in future one application can have more than one
 * 					status table, these table will be listed by List. Moreover, we define one status transition
 * 					table and one action table associate with the status table.
 * Input 			ofconn openflow connection
 * 					ofp_header pointer to the openflow header, the data is just behind the header, you can find the
 * 					payload by "pointer to header" + sizeof( struct ofp_header )
 * Output			1	means error in initialize the status table
 * 					0	means the pkt is successfully initialize the status table
 */
enum sfaerr sfa_msg_init(struct ofconn *ofconn, const struct ofp_header *sfah )
{
	printf("-----sfa enter init procedure!\n");


	int i = 0; //inner use
	char * cursor = NULL;
	uint32_t len = 0 ;


	if( g_apps.islistinit == false )
	{
		ovs_list_init(&g_apps.appslist);
		g_apps.islistinit = true;
	}

	//alloc app
	struct CONTROLLAPP* app = xmalloc( sizeof(struct CONTROLLAPP) );
	struct sfa_msg_init_st* pst =(struct sfa_msg_init_st*) ( (char*)sfah+sizeof( struct ofp_header));
	cursor = (char*)pst+sizeof( *pst );

	VLOG_INFO("++++++pjq, sizeof struct CONTROLLAPP: %ld, sizeof struct ofp_header: %ld", sizeof(struct CONTROLLAPP),
              sizeof(struct ofp_header));

	//set appid
	app->appid = ntohl(pst->aid);
	VLOG_INFO("+++++++ pjq appid: %d", app->appid);

	struct CONTROLLAPP* tmp_app = NULL;
	LIST_FOR_EACH(tmp_app , node, &g_apps.appslist)
	{
		if( tmp_app->appid == app->appid)
		{
			printf("-----sfa init the same table, so return now ! -----\n");
			free(app);
			app = NULL;
			return 0;
		}
	}

	//alloc st table
	struct STATUS_TABLE* st = xmalloc( sizeof(*st) );
	//init hmap st_entrys
	hmap_init( &st->st_entrys );
	int loop_ct = ntohl(pst->counts);
	//get & set match_bitmap
//	st->match_bitmap = ntohll(pst->mmp);
//    st->st_match = pst->st_match;
    st->st_match.field_id = ntohs(pst->st_match.field_id);
    st->st_match.offset = ntohs(pst->st_match.offset);
    st->st_match.len = ntohs(pst->st_match.len);

    VLOG_INFO("+++++ pjq st match, field id: %d, offset: %d, len: %d", st->st_match.field_id,
    		  st->st_match.offset, st->st_match.len);

    VLOG_INFO("+++++ pjq before init st, loop_ct: %d", loop_ct);
	if( loop_ct > 0 )
	{
		for( ; i < loop_ct ; i++)
			{
				struct ST_MATCH_ENTRY* sme = xmalloc( sizeof(*sme));
				sme->last_status = ntohl(*(uint32_t*)cursor);
				cursor = cursor+sizeof(uint32_t);
				len =ntohl(*(uint32_t*)cursor);
				char* data = xmalloc( len + 2 );
				data[len+1]='\0';
				data[len] ='\0';
				memcpy((void*)data,(void*)(cursor+sizeof(uint32_t)),len);
				sme->data = data;
				cursor = cursor+sizeof(uint32_t)+len;
				printf("add to st stat is : %ld,data is %s \n",sme->last_status,sme->data);
				hmap_insert(&st->st_entrys,&sme->node,hash_string(sme->data,0));
			}
	}

	//cursor point to the stt and alloc stt
	struct STATUS_TRANZ_TABLE* stt = xmalloc( sizeof(*stt));
	ovs_list_init(&stt->stt_entrys);





    uint32_t stt_count = ntohl(*(uint32_t*)cursor);
	struct sfa_msg_init_stt* stt_tmp = (struct sfa_msg_init_stt*)(cursor+sizeof(uint32_t));


    char* test_msg = cursor + 4;
    struct ds s;
    ds_init(&s);
    ds_put_hex_dump(&s, test_msg, 96*7, 0, false);
    VLOG_INFO("++++++ pjq before init stt, msg: \n%s", ds_cstr(&s));
    ds_destroy(&s);
//    xsleep(10);

    i = 0;
	VLOG_INFO("++++++pjq before init stt, stt_count: %d", stt_count);
	VLOG_INFO("++++++pjq sizeof sfa_msg_init_stt: %d", sizeof(stt_tmp));



	for( ; i < stt_count;i++)
	{
        test_msg = (char *)stt_tmp;
        ds_init(&s);
        ds_put_hex_dump(&s, test_msg, 96, 0, false);
        VLOG_INFO("++++++ pjq before init stt, i: %d, msg: \n%s", i, ds_cstr(&s));
        ds_destroy(&s);
		struct STT_MATCH_ENTRY* sttme = xmalloc( sizeof(*sttme) );
		VLOG_INFO("+++++++pjq sizeof sttme: %ld", sizeof(*sttme));
		VLOG_INFO("+++++++pjq sizeof struct sfa_msg_init_stt: %ld", sizeof(struct sfa_msg_init_stt));
		VLOG_INFO("+++++++pjq test_msg: %p", test_msg);
		sttme->cur_status = ntohl(stt_tmp->cur_status);
		sttme->oprator = ntohl(stt_tmp->oprator);
		sttme->param_left = ntohll(stt_tmp->param_left);
		sttme->param_left_match = stt_tmp->param_left_match;
		sttme->param_right = ntohll(stt_tmp->param_right);
		sttme->param_right_match = stt_tmp->param_right_match;
		sttme->last_status = ntohl(stt_tmp->last_status);
		VLOG_INFO("add to stt left param is %lu, right param is %lu,op : %d, last: %d next : %d \n",
				sttme->param_left,sttme->param_right,sttme->oprator,sttme->last_status,sttme->cur_status);
		ovs_list_insert(&stt->stt_entrys,&sttme->node);
		stt_tmp++;
	}

	//cur point to the at and alloc at
	cursor = (char*)stt_tmp;
	struct ACTION_TABLE* at = xmalloc( sizeof(*at) );
	hmap_init(&at->at_entrys);
	struct sfa_msg_init_at* at_tmp = (struct sfa_msg_init_at* )cursor;
	loop_ct = ntohl(at_tmp->counts);
//	at->bitmap = ntohll(at_tmp->bitmap);
//    at->at_match = at_tmp->at_match;
    at->at_match.field_id = ntohs(at_tmp->at_match.field_id);
    at->at_match.offset = ntohs(at_tmp->at_match.offset);
    at->at_match.len = ntohs(at_tmp->at_match.len);

	VLOG_INFO("+++++ pjq at match, field id: %d, offset: %d, len: %d", at->at_match.field_id,
			  at->at_match.offset, at->at_match.len);


    test_msg = cursor;
    ds_init(&s);
    ds_put_hex_dump(&s, test_msg, 12, 0, false);
    VLOG_INFO("++++++ pjq before init at, msg: \n%s", ds_cstr(&s));
    ds_destroy(&s);

	cursor = cursor+sizeof(struct sfa_msg_init_at);
	i = 0 ;
	VLOG_INFO("++++++pjq before init at, at_count: %d", loop_ct);



	for( ; i < loop_ct ; i++)
	{
	    VLOG_INFO("++++++ pjq in init at, i = %d", i);
		struct AT_MATCH_ENTRY* ame = xmalloc( sizeof(*ame));
		ame->act.actype = ntohl( ((struct SFA_ACTION*)cursor)->actype);
		ame->act.acparam = ntohl(((struct SFA_ACTION*)cursor)->acparam);
		ame->last_status = ntohl(*(uint32_t*)(cursor+sizeof( struct SFA_ACTION )));
		len = ntohl(*(uint32_t*)(cursor+sizeof( struct SFA_ACTION )+sizeof(uint32_t)));
		//alloc the data to store data string
		ame->data = xmalloc(len+2);
//		ame->data[len-1] = '\0';
//		ame->data[len] = '\0';
		memcpy(ame->data,cursor+sizeof(struct SFA_ACTION)+2*sizeof(uint32_t) , len);
		printf("add to at action type : %ld, action param : %ld , last status : %ld,data is %s \n",
				ame->act.actype,ame->act.acparam,ame->last_status,ame->data);
		hmap_insert( &at->at_entrys, &ame->node,hash_string(ame->data,0));
		cursor += sizeof(struct SFA_ACTION)+ 2*sizeof(uint32_t)+len;
	}

	//add st stt at to appcontroll
	app->pat = at;
	app->pst = st;
	app->pstt = stt;

	// insert app into g_apps;
	ovs_list_insert(&(g_apps.appslist) , &(app->node));

	VLOG_INFO("++++++pjq return sp msg process ok");
	return SFA_MSG_PROCESS_OK;


}

enum sfaerr sfa_msg_st_mod(struct ofconn *ofconn, const struct ofp_header *sfah)
{
	printf("enter sfa st mod !\n");

	struct sfa_msg_mod* pmsg = (char*)sfah+sizeof(struct ofp_header);

	uint32_t aid = ntohl(pmsg->appid);

	if(g_apps.islistinit == false )
		return SFA_MSG_PROCESS_ERROR;

	struct CONTROLLAPP* app = NULL;
	bool bfound = false;

	LIST_FOR_EACH(app , node, &g_apps.appslist)
	{
		if( app->appid == aid)
		{
			bfound = true;
			break;
		}
	}

	if( bfound == false )
		return SFA_MSG_PROCESS_ERROR;
	struct STATUS_TABLE* st = app->pst;

	int count = ntohl(pmsg->count);

	VLOG_INFO("+++++++pjq st mod count: %d", count);

	char* cursor = (char*)pmsg+2*sizeof(uint32_t);

    int i = 0;
    uint32_t status_tmp = 0 ;
    uint32_t len_tmp = 0 ;
    char * data_tmp = NULL;
    struct ST_MATCH_ENTRY* stmatch_tmp = NULL;
    for( ; i < count;i++){

        int tmt_tmp = ntohl(*( int *)cursor);
        status_tmp = 0 ;
        len_tmp = 0;
        data_tmp= NULL;
        if( tmt_tmp == ENTRY_ADD )
        {
            //do add entry
            status_tmp = ntohl(*(uint32_t*)( cursor+sizeof(uint32_t)));
            len_tmp = ntohl(*(uint32_t*)( cursor+2*sizeof(uint32_t)));
            VLOG_INFO("+++++ len tmp: %d, status: %d", len_tmp, status_tmp);
            data_tmp = xmalloc(len_tmp+2);
            data_tmp[len_tmp+1] = '\0';
            data_tmp[len_tmp] = '\0';
            VLOG_INFO("++++++ pjq data tmp: %s, len tmp: %d", data_tmp, len_tmp);
            memcpy(data_tmp , cursor + 3*sizeof(uint32_t),len_tmp);
            if( hmap_first_with_hash(&(st->st_entrys),hash_string(data_tmp,0)) != NULL)
            {
                printf("BUG--CHECK find dup entry while adding !\n");
                free(data_tmp);
                data_tmp = NULL;
                cursor =cursor+ 3*sizeof(uint32_t)+8;
                continue;
            }
            stmatch_tmp = xmalloc(sizeof(*stmatch_tmp));
            stmatch_tmp->last_status = status_tmp;
            stmatch_tmp->data = data_tmp;
            printf("st-mod  op is add , is %s ,  status is: %d\n",data_tmp,status_tmp);
            hmap_insert(&(st->st_entrys),&(stmatch_tmp->node),hash_string(data_tmp,0));
            cursor =cursor+ 3*sizeof(uint32_t)+8;

        }else if( tmt_tmp == ENTRY_UPDATE)
        {
            //do update
            status_tmp = ntohl(*(uint32_t*)( cursor+sizeof(uint32_t)));
            len_tmp = ntohl(*(uint32_t*)( cursor+2*sizeof(uint32_t)));
            data_tmp = xmalloc(len_tmp+2);
            data_tmp[len_tmp+1] = '\0';
            data_tmp[len_tmp] = '\0';
            memcpy(data_tmp , cursor+3*sizeof(uint32_t),len_tmp);
            stmatch_tmp = (struct ST_MATCH_ENTRY* )hmap_first_with_hash(&(st->st_entrys),hash_string(data_tmp,0));
            if( stmatch_tmp == NULL)
            {
                printf("BUG--CHECK can not find entry while update st!\n");
                free(data_tmp);
                data_tmp = NULL;
                cursor =cursor+ 3*sizeof(uint32_t)+len_tmp;
                continue;
            }
            printf("st-mod  op is update , data is %s , new status is: %ld\n",data_tmp,status_tmp);
            stmatch_tmp->last_status = status_tmp;
            cursor =cursor+ 3*sizeof(uint32_t)+len_tmp;

        }else if(tmt_tmp == ENTRY_DEL){
            //do del
            len_tmp = ntohl(*(uint32_t*)( cursor+2*sizeof(uint32_t)));
            data_tmp = xmalloc(len_tmp+2);
            data_tmp[len_tmp+1] = '\0';
            data_tmp[len_tmp] = '\0';
            memcpy(data_tmp , cursor+3*sizeof(uint32_t),len_tmp);
            stmatch_tmp = (struct ST_MATCH_ENTRY* )hmap_first_with_hash(&(st->st_entrys),hash_string(data_tmp,0));
            if( stmatch_tmp == NULL)
            {
                printf("BUG--CHECK can not find entry while del st!\n");
                free(data_tmp);
                data_tmp = NULL;
                cursor =cursor+ 3*sizeof(uint32_t)+len_tmp;
                continue;
            }
            printf("st-mod  op is del , data is %s\n",data_tmp);
            hmap_remove(&(st->st_entrys),stmatch_tmp);
            cursor =cursor+ 3*sizeof(uint32_t)+len_tmp;

        }else
        {
            printf("BUG-CHECK! invalid st mode type!\n");
            return SFA_MSG_PROCESS_ERROR;
        }

    }

    printf("leaving st mod done ok! \n");
    return SFA_MSG_PROCESS_OK;

}

enum sfaerr sfa_msg_at_mod(struct ofconn *ofconn, const struct ofp_header *sfah)
{
	    printf("enter sfa at mod !\n");

		struct sfa_msg_mod* pmsg = (char*)sfah+sizeof(struct ofp_header);
		uint32_t aid = ntohl(pmsg->appid);

		if(g_apps.islistinit == false )
			return SFA_MSG_PROCESS_ERROR;

		struct CONTROLLAPP* app = NULL;
		bool bfound = false;

		LIST_FOR_EACH(app , node, &g_apps.appslist)
		{
			if( app->appid == aid)
			{
				bfound = true;
				break;
			}
		}

		if( bfound == false )
			return SFA_MSG_PROCESS_ERROR;

		struct ACTION_TABLE* at = app->pat;

		uint32_t count = ntohl(pmsg->count);
		char* cursor = (char*)pmsg+2*sizeof(uint32_t);

        VLOG_INFO("+++++++pjq at mod count: %d", count);

		int i = 0;
		uint32_t status_tmp = 0 ;
		uint32_t len_tmp = 0 ;
//		char * data_tmp = NULL;
		struct AT_MATCH_ENTRY* atmatch_tmp = NULL;
		struct SFA_ACTION* sact_tmp = NULL;

	    struct sfa_msg_mod_at* at_tmp = (struct sfa_msg_mod_at*)(cursor) ;

		for( ; i < count;i++){

			VLOG_INFO("++++++pjq in st mod, i = %d", i);

			int tmt_tmp = ntohl(at_tmp->type);

//			uint32_t tmt_tmp = *(uint32_t*)cursor;
			status_tmp = 0 ;
//			len_tmp = 0;
//			data_tmp= NULL;
            len_tmp = ntohl(at_tmp->len);
            char* data_tmp = xmalloc(len_tmp + 2);
			if( tmt_tmp == ENTRY_ADD )
			{
				//do add entry
//				sact_tmp = (struct SFA_ACTION*)(cursor+sizeof(uint32_t));
//				status_tmp = ntohl(*(uint32_t*)( cursor+sizeof(uint32_t)+ sizeof(struct SFA_ACTION)));
//				len_tmp = ntohl(*(uint32_t*)( cursor+sizeof(struct SFA_ACTION)+2*sizeof(uint32_t)));
//				data_tmp = xmalloc(len_tmp+2);
//				data_tmp[len_tmp+1] = '\0';
//				data_tmp[len_tmp] = '\0';
//				memcpy(data_tmp , cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t),len_tmp);
				// we should not find the dup in at , cause hash is the bitmatch string ,but one bitmatch string can have multiple actions based on different status
				//data_tmp = cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t);
//				if( hmap_first_with_hash(&(at->at_entrys),hash_string(data_tmp,0)) != NULL)
//					{
//					printf("BUG--CHECK find dup entry while adding Action Table !\n");
//					free(data_tmp);
//					data_tmp=NULL;
//					cursor = cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t)+len_tmp;
//					continue;
//					}
//                len_tmp = ntohl(at_tmp->len);
//                data_tmp = xmalloc(len_tmp + 2);
                data_tmp[len_tmp+1] = '\0';
                data_tmp[len_tmp] = '\0';
//                memcpy(data_tmp ,at_tmp->value,len_tmp);
                memcpy(data_tmp , cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t),len_tmp);
				atmatch_tmp = xmalloc(sizeof(*atmatch_tmp));
//				atmatch_tmp->last_status = status_tmp;
				atmatch_tmp->last_status = ntohl(at_tmp->status);
				atmatch_tmp->act.actype = ntohl(at_tmp->act_type);
				atmatch_tmp->act.acparam = ntohl(at_tmp->act_param);
				atmatch_tmp->data = data_tmp;
				printf("at-mod  op is add , new data is %s , new status is: %d, new actype : %d , new actparam : %d \n",
						atmatch_tmp->data,atmatch_tmp->last_status,atmatch_tmp->act.actype,atmatch_tmp->act.acparam);
//				uint64_t p[1] = {at_tmp->value};
				hmap_insert(&(at->at_entrys),&(atmatch_tmp->node),hash_string(data_tmp, 0));
				VLOG_INFO("+++++pjq after hmap insert");
			    cursor =cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t)+8;
				at_tmp++;

			}else if( tmt_tmp == ENTRY_UPDATE)
			{
				//do update
				sact_tmp = (struct SFA_ACTION*)(cursor+sizeof(uint32_t));
				status_tmp = ntohl(*(uint32_t*)( cursor+sizeof(uint32_t)+ sizeof(struct SFA_ACTION)));
				len_tmp = ntohl(*(uint32_t*)( cursor+sizeof(struct SFA_ACTION)+2*sizeof(uint32_t)));
				data_tmp = xmalloc(len_tmp+2);
				data_tmp[len_tmp+1] = '\0';
				data_tmp[len_tmp] = '\0';
				memcpy(data_tmp , cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t),len_tmp);
				//data_tmp = cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t);
				atmatch_tmp = (struct AT_MATCH_ENTRY* )hmap_first_with_hash(&(at->at_entrys),hash_string(data_tmp,0));
				if( atmatch_tmp == NULL)
				{
					printf("BUG--CHECK can not find entry while update Action Table!\n");
					free(data_tmp);
					data_tmp=NULL;
					cursor = cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t)+len_tmp;
					continue;
				}
				printf("at-mod  op is update , data is %s , new status is: %ld,new actype : %ld , new actparam : %ld\n",
						data_tmp,status_tmp,ntohl(sact_tmp->actype),ntohl(sact_tmp->acparam));
				atmatch_tmp->last_status = status_tmp;
				atmatch_tmp->act.acparam = ntohl(sact_tmp->acparam);
				atmatch_tmp->act.actype = ntohl(sact_tmp->actype);
				cursor = cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t)+len_tmp;

			}else if(tmt_tmp == ENTRY_DEL){
				//do del
				//data_tmp = cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t);
				len_tmp = ntohl(*(uint32_t*)( cursor+sizeof(struct SFA_ACTION)+2*sizeof(uint32_t)));
				data_tmp = xmalloc(len_tmp+2);
				data_tmp[len_tmp+1] = '\0';
				data_tmp[len_tmp] = '\0';
				memcpy(data_tmp , cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t),len_tmp);
				atmatch_tmp = (struct AT_MATCH_ENTRY*)hmap_first_with_hash(&at->at_entrys,hash_string(data_tmp,0));
				if( atmatch_tmp == NULL)
				{
					printf("BUG--CHECK can not find entry while del Action Table!\n");
					free(data_tmp);
					data_tmp=NULL;
					cursor = cursor+sizeof(struct SFA_ACTION)+3*sizeof(uint32_t)+len_tmp;
					continue;
				}
				printf("at-mod  op is del , data is %s \n",data_tmp);
				hmap_remove(&(at->at_entrys),&(atmatch_tmp->node));

			}else
			{
				printf("BUG-CHECK! invalid at mode type!\n");
				return SFA_MSG_PROCESS_ERROR;
			}

		}

		printf("leaving at mod !\n");
		return SFA_MSG_PROCESS_OK;


}

/*
 * Description		called in handle_openflow__ procedure, the function first check the msg type
 * 					if it is sfamsg , then process the msg else it returns . The error of sfaerror
 * 					type will not pass out, it will only be used by inner function to do self-check.
 * Input 			ofconn openflow connection
 * 					ofpbuf the puf point to the openflow msg
 * Output			1	means the pkt should be handled by original openflow procedure
 * 					0	means the pkt is successfully handled by sfa module
 */
int sfa_handle_pkt(struct ofconn *ofconn, const struct ofpbuf *msg)
{
		const struct ofp_header *oh = msg->data;
	    enum sfaerr error;

	    //printf("In sfa_handle_pkt! type is %d \n",oh->type);

	    printf("-----sfa msg type is %d-----\n",oh->type);

	    switch( oh->type )
	    {
	    case OFPTYPE_SFA_TABLE_CREATE:
	    	error = sfa_msg_init(ofconn,oh);
	    	break;
	    case OFPTYPE_SFA_ST_ENTRY_MOD:
	    	error = sfa_msg_st_mod(ofconn,oh);
	    	break;
	    case OFPTYPE_SFA_AT_ENTRY_MOD:
	    	error = sfa_msg_at_mod(ofconn,oh);
	    	break;
	    default:
	    	//printf("non-sfa msg return to openflow!\n");
	    	return -1 ;
	    }

	    if( error == 0 )
	    {
	    	printf("successfully handle the sfa msg\n");
	    	return 0;
	    }else
	    {
	    	printf("###BUG-CHECK### fail to  handle the sfa msg\n");
	    	return -1;
	    }



}

void cnm(){

}
