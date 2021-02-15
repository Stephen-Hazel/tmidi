// tmidi - dump alsa info

#include "../stv/os.h"
#include <alsa/asoundlib.h>


__attribute__((noreturn))
void DieSnd (char *msg, int err)
{  DBG ("ALSA `s error: `s", msg, snd_strerror (err));   exit (99);  }


static void list_dev (snd_ctl_t *ctl, int card, int dev)
{ int err;
  unsigned int isub, osub, nsub, s;
  const char  *name, *sname;
  snd_rawmidi_info_t *inf;
   snd_rawmidi_info_alloca   (& inf);
   snd_rawmidi_info_set_device (inf, SC(unsigned int,dev));

   isub = 0;
   snd_rawmidi_info_set_stream     (inf, SND_RAWMIDI_STREAM_INPUT);
   if (! snd_ctl_rawmidi_info (ctl, inf))
      isub = snd_rawmidi_info_get_subdevices_count (inf);
   osub = 0;
   snd_rawmidi_info_set_stream     (inf, SND_RAWMIDI_STREAM_OUTPUT);
   if (! snd_ctl_rawmidi_info (ctl, inf))
      osub = snd_rawmidi_info_get_subdevices_count (inf);

   nsub = (isub > osub) ? isub : osub;
   if (! nsub)  return;

   if (nsub == 1) {
      snd_rawmidi_info_set_stream (inf, isub ? SND_RAWMIDI_STREAM_INPUT
                                             : SND_RAWMIDI_STREAM_OUTPUT);
      snd_rawmidi_info_set_subdevice (inf, 0);
      if ((err =     snd_ctl_rawmidi_info (ctl, inf)))
         DieSnd (CC("snd_ctl_rawmidi_info"), err);
      name = snd_rawmidi_info_get_name (inf);
      DBG ("`s  (`c`c  hw:`d,`d)", name, isub ? 'I':' ',
                                         osub ? 'O':' ', card, dev);
      return;
   }
   for (s = 0;  s < nsub;  s++) {
      snd_rawmidi_info_set_stream (inf, s < isub ? SND_RAWMIDI_STREAM_INPUT
                                                 : SND_RAWMIDI_STREAM_OUTPUT);
      snd_rawmidi_info_set_subdevice (inf, s);
      if ((err =     snd_ctl_rawmidi_info (ctl, inf)))
         DieSnd (CC("snd_ctl_rawmidi_info"), err);
      name  = snd_rawmidi_info_get_name           (inf);
      sname = snd_rawmidi_info_get_subdevice_name (inf);
      DBG ("`s  (`c`c  hw:`d,`d,`d  devName=`s)",
           sname, s < isub ? 'I':' ',
                  s < osub ? 'O':' ', card, dev, s, name);
   }
}


int main (int argc, char *argv [])
{ ubyte ev [3];
  int   err, card, dev;
  TStr  name;
  snd_rawmidi_t *mo;
  snd_ctl_t     *ctl;
   if (argc > 1) {
      StrCp (name, argv [1]);
      if ((err =     snd_rawmidi_open (
                                    nullptr, & mo, name, SND_RAWMIDI_NONBLOCK)))
         DieSnd (CC("snd_rawmidi_open"), err);

      ev [0] = 0x90;   ev [1] = 60;   ev [2] = 100;   // chan 0 middle c
      snd_rawmidi_write (mo, ev, 3);   snd_rawmidi_drain (mo);
      sleep (1);                       ev [2] = 0;
      snd_rawmidi_write (mo, ev, 3);   snd_rawmidi_drain (mo);

      ev [0] = 0x99;   ev [1] = 38;   ev [2] = 100;   // chan 10 snare
      snd_rawmidi_write (mo, ev, 3);   snd_rawmidi_drain (mo);
      sleep (1);                       ev [2] = 0;
      snd_rawmidi_write (mo, ev, 3);   snd_rawmidi_drain (mo);

      snd_rawmidi_close (mo);
      return 0;
   }
   for (card = -1;;) {
      if ((err = snd_card_next (& card)))  DieSnd (CC("snd_card_next"), err);
      if (card < 0)  break;

      StrFmt (name, "hw:`d", card);
//DBG("card=`d name=`s", card, name);;
      if ((err =     snd_ctl_open (& ctl, name, 0)))
         DieSnd (CC("snd_ctl_open"), err);
      for (dev = -1;;) {
         if ((err =     snd_ctl_rawmidi_next_device (ctl, & dev)))
            DieSnd (CC("snd_ctl_rawmidi_next_device"), err);
         if (dev < 0)  break;

         list_dev (ctl, card, dev);
      }
      snd_ctl_close (ctl);
   }
   return 0;
}
