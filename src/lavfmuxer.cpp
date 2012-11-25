/*  dvbcut
    Copyright (c) 2005 Sven Over <svenover@svenover.de>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* $Id$ */

extern "C" {
#include <avformat.h>
#include <avcodec.h>
}
#include <cstring>
#include <utility>
#include <list>
#include "avframe.h"
#include "streamhandle.h"
#include "lavfmuxer.h"

#include <stdio.h>

lavfmuxer::lavfmuxer(const char *format, uint32_t audiostreammask, mpgfile &mpg, const char *filename)
    : muxer(), avfc(0), fileopened(false)
  {
  fmt = av_guess_format(format, NULL, NULL);
  if (!fmt) {
    return;
    }

  avfc=avformat_alloc_context();
  if (!avfc)
    return;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 35, 0)
// todo: what here ?
//  maybe: AVFormatContext::audio_preload but no direct access.
//    AVOptions
//    iformat
#else
  avfc->preload= (int)(.5*AV_TIME_BASE);
  avfc->mux_rate=10080000;
#endif
  avfc->max_delay= (int)(.7*AV_TIME_BASE);

  avfc->oformat=fmt;
  strncpy(avfc->filename, filename, sizeof(avfc->filename));

  int id=0;

  st[VIDEOSTREAM].stream_index=id;
  AVStream *s=st[VIDEOSTREAM].avstr=avformat_new_stream(avfc, NULL);
  strpres[VIDEOSTREAM]=true;
  av_free(s->codec);
  mpg.setvideoencodingparameters();
  s->codec=mpg.getavcc(VIDEOSTREAM);
  s->codec->rc_buffer_size = 224*1024*8;
#if LIBAVFORMAT_VERSION_INT >= ((52<<16)+(21<<8)+0)
  s->sample_aspect_ratio = s->codec->sample_aspect_ratio;
#endif

  for (int i=0;i<mpg.getaudiostreams();++i)
    if (audiostreammask & (1u<<i)) {
      int astr=audiostream(i);
      st[astr].stream_index=id;
      s=st[astr].avstr=avformat_new_stream(avfc, NULL);
      strpres[astr]=true;
      if (s->codec)
        av_free(s->codec);
      s->codec = avcodec_alloc_context3(NULL);
      avcodec_get_context_defaults3(s->codec, NULL);
      s->codec->codec_type=AVMEDIA_TYPE_AUDIO;
      s->codec->codec_id = (mpg.getstreamtype(astr)==streamtype::ac3audio) ?
	CODEC_ID_AC3 : CODEC_ID_MP2;
      s->codec->rc_buffer_size = 224*1024*8;

      // Must read some packets to get codec parameters
      streamhandle sh(mpg.getinitialoffset());
      streamdata *sd=sh.newstream(astr,mpg.getstreamtype(astr),mpg.istransportstream());

      while (sh.fileposition < mpg.getinitialoffset()+(4<<20)) {
	if (mpg.streamreader(sh)<=0)
	  break;

	if (sd->getitemlistsize() > 1) {
	  if (!avcodec_open2(s->codec,
			     avcodec_find_decoder(s->codec->codec_id), NULL)) {
	    int16_t samples[AVCODEC_MAX_AUDIO_FRAME_SIZE/sizeof(int16_t)];
	    int frame_size=sizeof(samples);
	    //fprintf(stderr, "** decode audio size=%d\n", sd->inbytes());
#if LIBAVCODEC_VERSION_INT > ((52<<16)+(25<<8)+0)
	    AVPacket pkt;
 	    av_init_packet( &pkt );
	    pkt.data = (uint8_t*) sd->getdata();
	    pkt.size = sd->inbytes();
	    avcodec_decode_audio3
	      (s->codec,samples,&frame_size, &pkt);
#else
#if LIBAVCODEC_VERSION_INT >= ((52<<16)+(0<<8)+0)
	    avcodec_decode_audio2
#else
	    avcodec_decode_audio
#endif
	      (s->codec,samples,&frame_size,
	       (uint8_t*) sd->getdata(),sd->inbytes());
	    avcodec_close(s->codec);
#endif
	  }
	  break;
	}
      }
    }

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 35, 0)
  // error: 'av_set_parameters' was not declared in this scope
  if (!(fmt->flags & AVFMT_NOFILE)&&(avio_open(&avfc->pb, filename, AVIO_FLAG_WRITE) < 0)) {
#else
  if ((av_set_parameters(avfc, NULL) < 0) || (!(fmt->flags & AVFMT_NOFILE)&&(url_fopen(&avfc->pb, filename, URL_WRONLY) < 0))) {
#endif  
    av_free(avfc);
    avfc=0;
    return;
    }
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 35, 0)
// todo: what here ?
//  maybe: AVFormatContext::audio_preload but no direct access.
//    AVOptions
//    iformat
#else
  avfc->preload= (int)(.5*AV_TIME_BASE);
  avfc->mux_rate=10080000;
#endif
  avfc->max_delay= (int)(.7*AV_TIME_BASE);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 35, 0)
  av_dump_format(avfc, 0, filename, 1);
  fileopened=true;
  avformat_write_header(avfc, NULL);
#else
  dump_format(avfc, 0, filename, 1);
  fileopened=true;
  av_write_header(avfc);
#endif
  }


lavfmuxer::~lavfmuxer()
  {
  if (avfc) {
    if (fileopened) {
      av_write_trailer(avfc);
      if (!(fmt->flags & AVFMT_NOFILE))
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 35, 0)
        avio_close(avfc->pb);
#elif LIBAVFORMAT_VERSION_INT >= ((52<<16)+(0<<8)+0)
        url_fclose(avfc->pb);
#else
        url_fclose(&avfc->pb);
#endif
      }

    av_free(avfc);
    }
  }

