// tmidi - dump alsa midi device info

#include "../stv/os.h"
#include "alsa/asoundlib.h"

int main ()
{ int  card, dev, sub, nsub, isub, osub, err;
  TStr name;
  snd_ctl_t          *ctl;
  snd_rawmidi_info_t *info;
  const char         *desc, *desc2;
   snd_rawmidi_info_alloca (& info);   // sheesh :/
   for (card = -1;;) {
      if ((err = snd_card_next (& card))) {
         DBG ("snd_card_next error: `s", snd_strerror (err));
         break;
      }
      if (card < 0)  break;

//DBG("card#=`d", card);
      StrFmt (name, "hw:`d", card);
      if ((err = snd_ctl_open (& ctl, name, 0)) < 0) {
         DBG ("snd_ctl_open error for card `d: `s", card, snd_strerror (err));
         break;
      }
      for (dev = -1;;) {
         if ((err = snd_ctl_rawmidi_next_device (ctl, & dev)) < 0) {
            DBG ("snd_ctl_rm_next_device error: `s", snd_strerror (err));
            break;
         }
         if (dev < 0)  break;

//DBG(" dev=`d", dev);
         isub = osub = 0;
         snd_rawmidi_info_set_device (info, dev);
         snd_rawmidi_info_set_stream (info, SND_RAWMIDI_STREAM_INPUT);
         if (snd_ctl_rawmidi_info (ctl, info) >= 0)
            isub = snd_rawmidi_info_get_subdevices_count (info);
         snd_rawmidi_info_set_stream (info, SND_RAWMIDI_STREAM_OUTPUT);
         if (snd_ctl_rawmidi_info (ctl, info) >= 0)
            osub = snd_rawmidi_info_get_subdevices_count (info);
         nsub = isub > osub ? isub : osub;
//DBG("  nsub=`d isub=`d osub=`d", nsub, isub, osub);
         for (sub = 0;  sub < nsub;  sub++) {
            snd_rawmidi_info_set_stream (
               info, sub < isub ? SND_RAWMIDI_STREAM_INPUT
                                : SND_RAWMIDI_STREAM_OUTPUT);
            snd_rawmidi_info_set_subdevice (info, sub);
            if ((err = snd_ctl_rawmidi_info (ctl, info))) {
               DBG ("snd_rm_info_set_subdevice error: `s\n",
                    snd_strerror (err));
               break;
            }
            desc  = snd_rawmidi_info_get_name           (info);
            desc2 = snd_rawmidi_info_get_subdevice_name (info);
            if ((sub == 0) && (nsub == 1))
                  DBG ("`c`c  hw:`d,`d  `s",
                       sub < isub ? 'I':' ',
                       sub < osub ? 'O':' ', card, dev, desc);
            else  DBG ("`c`c  hw:`d,`d,`d  `s",
                       sub < isub ? 'I':' ',
                       sub < osub ? 'O':' ', card, dev, desc2);
         }
      }
      snd_ctl_close (ctl);
   }
   return 0;
}
