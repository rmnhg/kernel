/*
* Copyright(C) 2011-2014 Foxconn International Holdings, Ltd. All rights reserved
*/
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/fih_hw_info.h>


static int proc_calc_metrics(char *page, char **start, off_t off,
                             int count, int *eof, int len)
{
  if (len <= off+count) *eof = 1;
  *start = page + off;
  len -= off;
  if (len > count) len = count;
  if (len < 0 ) len = 0;
  return len;
}

static int device_model_read_proc(char *page, char **start, off_t off,
                                  int count, int *eof, void *data)
{
  int len, i;
  unsigned int project = fih_get_product_id();
  char ver[16]= {0} ;

  for(i=0; project_id_map[i].ID != PROJECT_MAX; i++)
  {
    if(project_id_map[i].ID == project)
    {
      strncpy(ver, project_id_map[i].STR ,project_id_map[i].LEN);
      ver[project_id_map[i].LEN]='\0';
      break;
    }
  }

  len = snprintf(page, count, "%s\n", ver);

  return proc_calc_metrics(page, start, off, count, eof, len);
}

static int phase_id_read_proc(char *page, char **start, off_t off,
                              int count, int *eof, void *data)
{
  int len, i;
  int phase = fih_get_product_phase();
  char ver[16]= {0};

  for(i=0; phase_id_map[i].ID != PHASE_MAX; i++)
  {
    if(phase_id_map[i].ID == phase)
    {
      strncpy(ver, phase_id_map[i].STR ,phase_id_map[i].LEN);
      ver[phase_id_map[i].LEN]='\0';
      break;
    }
  }

  len = snprintf(page, count, "%s\n", ver);

  return proc_calc_metrics(page, start, off, count, eof, len);
}

static int band_read_proc(char *page, char **start, off_t off,
                          int count, int *eof, void *data)
{
  int len, i;
  int band = fih_get_band_id();
  char ver[16]= {0};

  for(i=0; band_id_map[i].ID != BAND_MAX; i++)
  {
    if(band_id_map[i].ID == band)
    {
      strncpy(ver, band_id_map[i].STR ,band_id_map[i].LEN);
      ver[band_id_map[i].LEN]='\0';
      break;
    }
  }

  len = snprintf(page, count, "%s\n", ver);

  return proc_calc_metrics(page, start, off, count, eof, len);
}

static int siminfo_read_proc(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
  int len, i;
  int sim = fih_get_sim_id();
  char ver[16]= {0} ;

  for(i=0; sim_id_map[i].ID != SIM_MAX; i++)
  {
    if(sim_id_map[i].ID == sim)
    {
      strncpy(ver, sim_id_map[i].STR ,sim_id_map[i].LEN);
      ver[sim_id_map[i].LEN]='\0';
      break;
    }
  }

  len = snprintf(page, count, "%s\n", ver);

  return proc_calc_metrics(page, start, off, count, eof, len);
}

static struct
{
  char *name;
  int (*read_proc)(char*,char**,off_t,int,int*,void*);
}*p, fih_info[] =
     {
       {"devmodel",         device_model_read_proc},
       {"phaseid",          phase_id_read_proc},
       {"bandinfo",         band_read_proc},
       {"siminfo",          siminfo_read_proc},
       {NULL,},
     };

void fih_info_init(void)
{
    for (p = fih_info; p->name; p++)
        create_proc_read_entry(p->name, 0, NULL, p->read_proc, NULL);
}
EXPORT_SYMBOL(fih_info_init);

void fih_info_remove(void)
{
    for (p = fih_info; p->name; p++)
        remove_proc_entry(p->name, NULL);

}
EXPORT_SYMBOL(fih_info_remove);
