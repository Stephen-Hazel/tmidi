// tmidi - dump alsa info

#include "../stv/os.h"
#include "alsa/asoundlib.h"

static void error (const char *format, ...)
{ va_list ap;
   va_start (ap, format);
   vfprintf (stderr, format, ap);
   va_end (ap);
   putc ('\n', stderr);
}


static void list_device (snd_ctl_t *ctl, int card, int device)
{ snd_rawmidi_info_t *info;
  const char *name;
  const char *sub_name;
  int subs, subs_in, subs_out;
  int sub;
  int err;
   snd_rawmidi_info_alloca (& info);
   snd_rawmidi_info_set_device (info, device);

   snd_rawmidi_info_set_stream (info, SND_RAWMIDI_STREAM_INPUT);
   err = snd_ctl_rawmidi_info (ctl, info);
   if (err >= 0)  subs_in = snd_rawmidi_info_get_subdevices_count (info);
   else           subs_in = 0;

   snd_rawmidi_info_set_stream (info, SND_RAWMIDI_STREAM_OUTPUT);
   err = snd_ctl_rawmidi_info (ctl, info);
   if (err >= 0)  subs_out = snd_rawmidi_info_get_subdevices_count (info);
   else           subs_out = 0;

   subs = subs_in > subs_out ? subs_in : subs_out;
   if (!subs)  return;

   for (sub = 0; sub < subs; ++sub) {
      snd_rawmidi_info_set_stream (
         info, sub < subs_in ? SND_RAWMIDI_STREAM_INPUT
                             : SND_RAWMIDI_STREAM_OUTPUT);
      snd_rawmidi_info_set_subdevice (info, sub);
      err = snd_ctl_rawmidi_info (ctl, info);
      if (err < 0) {
         error ("cannot get rawmidi information %d:%d:%d: %s\n",
                 card, device, sub, snd_strerror (err));
         return;
      }
      name = snd_rawmidi_info_get_name (info);
      sub_name = snd_rawmidi_info_get_subdevice_name (info);
      if (sub == 0 && sub_name [0] == '\0') {
         printf ("%c%c  hw:%d,%d    %s",
                sub < subs_in  ? 'I' : ' ',
                sub < subs_out ? 'O' : ' ',  card, device, name);
         if (subs > 1)  printf (" (%d subdevices)", subs);
         putchar ('\n');
         break;
      }
      else
         printf ("%c%c  hw:%d,%d,%d  %s\n",
                sub < subs_in  ? 'I' : ' ',
                sub < subs_out ? 'O' : ' ',  card, device, sub, sub_name);
   }
}


static void list_card_devices (int card)
{ snd_ctl_t *ctl;
  char name [32];
  int  device;
  int  err;
DBG("card#=`d", card);
   sprintf (name, "hw:%d", card);
   if ((err = snd_ctl_open (&ctl, name, 0)) < 0) {
      error ("cannot open control for card %d: %s", card, snd_strerror (err));
      return;
   }
   device = -1;
   for (;;) {
      if ((err = snd_ctl_rawmidi_next_device (ctl, &device)) < 0) {
         error ("cannot determine device number: %s", snd_strerror (err));
         break;
      }
      if (device < 0)  break;
      list_device (ctl, card, device);
   }
   snd_ctl_close (ctl);
}


static void device_list (void)
{ int card, err;
   card = -1;
   if ((err = snd_card_next (& card)) < 0)
      {error ("cannot determine card number: %s", snd_strerror (err));  return;}
   if (card < 0)  {error ("no sound card found");   return;}
   puts ("Dir Device    Name");
   do {
      list_card_devices (card);
      if ((err = snd_card_next (& card)) < 0) {
         error ("cannot determine card number: %s", snd_strerror (err));  break;
      }
   }
   while (card >= 0);
}


static void rawmidi_list (void)
// dump super ugly alsa config of rawmidi
{ snd_output_t *output;
  snd_config_t *config;
  int           err;
   if ((err = snd_config_update ()) < 0) {
      error ("snd_config_update failed: %s",       snd_strerror (err));  return;
   }
   if ((err = snd_output_stdio_attach (&output, stdout, 0)) < 0) {
      error ("snd_output_stdio_attach failed: %s", snd_strerror (err));  return;
   }
   if (snd_config_search (snd_config, "rawmidi", &config) >= 0)
      {puts ("RawMIDI list:");   snd_config_save (config, output);}
   snd_output_close (output);
}


int main ()  {device_list ();   rawmidi_list ();   return 0;}
