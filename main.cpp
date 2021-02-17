// tmidi - dump alsa info n test in/output  (just testin out amidi.c)
// tmidi          lists hw:card/dev/subs, i/o, and names
// tmidi hw7,0    sends a middle c and snare hit to card/dev/sub
// tmidi hw7,0 i  prints midi bytes (in hex) from the device until ctrl-c

#include "../stv/os.h"
#include <signal.h>
#include <alloca.h>                    // sighhh
#include <alsa/asoundlib.h>

static int Stop;

void sig_handler (int signum)  {(void)signum;   Stop = 1;}


__attribute__((noreturn))
void DieSnd (char *msg, int err)
{  DBG ("ALSA `s error: `s", msg, snd_strerror (err));   exit (99);  }


int main (int argc, char *argv [])
{ unsigned int isub, osub, nsub, s;
  int          card, dev, err;
  TStr         name;
  snd_ctl_t          *ctl;
  snd_rawmidi_info_t *inf;
  snd_rawmidi_t      *mi, *mo;
   if (argc < 2)                       // list card/dev/subs
      for (card = -1;;) {
         if ((err = snd_card_next (& card)))  DieSnd (CC("snd_card_next"), err);
         if (card < 0)  break;

         StrFmt (name, "hw:`d", card);
//DBG("card=`d name=`s", card, name);
         if ((err =     snd_ctl_open (& ctl, name, 0)))
            DieSnd (CC("snd_ctl_open"), err);
         for (dev = -1;;) {
            if ((err =     snd_ctl_rawmidi_next_device (ctl, & dev)))
               DieSnd (CC("snd_ctl_rawmidi_next_device"), err);
            if (dev < 0)  break;

//DBG(" dev=`d", dev);
            snd_rawmidi_info_alloca   (& inf);   // linux !!!  :(
            snd_rawmidi_info_set_device (inf, SC(unsigned int,dev));
            isub = osub = 0;
            snd_rawmidi_info_set_stream     (inf, SND_RAWMIDI_STREAM_INPUT);
            if (! snd_ctl_rawmidi_info (ctl, inf))
               isub = snd_rawmidi_info_get_subdevices_count (inf);
            snd_rawmidi_info_set_stream     (inf, SND_RAWMIDI_STREAM_OUTPUT);
            if (! snd_ctl_rawmidi_info (ctl, inf))
               osub = snd_rawmidi_info_get_subdevices_count (inf);
            nsub = (isub > osub) ? isub : osub;
            for (s = 0;  s < nsub;  s++) {
               snd_rawmidi_info_set_stream (
                  inf, s < isub ? SND_RAWMIDI_STREAM_INPUT
                                : SND_RAWMIDI_STREAM_OUTPUT);
               snd_rawmidi_info_set_subdevice (inf, s);
               if ((err =     snd_ctl_rawmidi_info (ctl, inf)))
                  DieSnd (CC("snd_ctl_rawmidi_info"), err);
               if (nsub == 1)
                     DBG ("  hw:`d,`d  "  "  (`c`c)  `s",
                          card, dev,      isub ? 'I':' ',   osub ? 'O':' ',
                          snd_rawmidi_info_get_name           (inf));
               else  DBG ("  hw:`d,`d,`d" "  (`c`c)  `s",
                          card, dev, s, s<isub ? 'I':' ', s<osub ? 'O':' ',
                          snd_rawmidi_info_get_subdevice_name (inf));
            }
         }
         snd_ctl_close (ctl);
      }
   else {
      StrCp (name, argv [1]);
      if (argc < 3) {                  // output test
         if ((err =     snd_rawmidi_open (nullptr, & mo, name,
                                                         SND_RAWMIDI_NONBLOCK)))
            DieSnd (CC("snd_rawmidi_open"), err);

        ubyte ev [3];
         ev [0] = 0x90;   ev [1] = 60;   ev [2] = 100;     // chan 1 middle c
         snd_rawmidi_write (mo, ev, 3);   snd_rawmidi_drain (mo);
         sleep (1);                       ev [2] = 0;
         snd_rawmidi_write (mo, ev, 3);   snd_rawmidi_drain (mo);

         ev [0] = 0x99;   ev [1] = 38;   ev [2] = 100;     // chan 10 snare
         snd_rawmidi_write (mo, ev, 3);   snd_rawmidi_drain (mo);
         sleep (1);                       ev [2] = 0;
         snd_rawmidi_write (mo, ev, 3);   snd_rawmidi_drain (mo);

         snd_rawmidi_close (mo);
      }
      else {                           // input test
        struct pollfd  *pfd;
        int             nfd;
        unsigned short  rev;
        unsigned char   buf [256];
         if ((err =     snd_rawmidi_open (& mi, nullptr, name,
                                                         SND_RAWMIDI_NONBLOCK)))
            DieSnd (CC("snd_rawmidi_open"), err);

         snd_rawmidi_read (mi, nullptr, 0);      // trigger reading
         nfd = snd_rawmidi_poll_descriptors_count (mi);
         pfd = SC(struct pollfd *,alloca (SC(unsigned int,nfd) *
                                                       sizeof (struct pollfd)));
         if ((err =     snd_rawmidi_poll_descriptors (
                                      mi, & pfd [0], SC(unsigned int,nfd))) < 0)
            DieSnd (CC("snd_rawmidi_poll_descriptors"), err);
         signal (SIGINT, sig_handler);      // ctrl-c sets Stop
         fprintf (stderr, "ctrl-c to exit\n");
         while (! Stop) {
            err = poll (pfd, SC(unsigned long,nfd), -1);
            if (Stop || ((err < 0) && (errno == EINTR)))     break;

            if (errno < 0)
               {DBG ("poll failed: `s", strerror (errno));   break;}

            if ((err =     snd_rawmidi_poll_descriptors_revents (
                                     mi, pfd, SC(unsigned int,nfd), & rev)) < 0)
               DieSnd (CC("snd_rawmidi_poll_descriptors_revents"), err);

            if (rev & (POLLERR | POLLHUP))  break;
            if (! (rev & POLLIN))           continue;

            err =         snd_rawmidi_read (mi, buf, sizeof (buf));
            if (err == -EAGAIN)  continue;
            if (err < 0)  DieSnd (CC("snd_rawmidi_read"), err);

           int i, len = 0;
            for (i = 0;  i < err;  i++)     // skip duuumb events
               if ((buf [i] != MIDI_CMD_COMMON_CLOCK) &&
                   (buf [i] != MIDI_CMD_COMMON_SENSING))  buf [len++] = buf [i];
            for (i = 0;  i < len;  i++)  printf ("%02X ", buf [i]);
            if (len)  {printf ("\n");   fflush (stdout);}
         }

         snd_rawmidi_close (mi);
      }
   }
   return 0;
}
