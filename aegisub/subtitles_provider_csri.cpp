// Copyright (c) 2007, Rodrigo Braz Monteiro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


///////////
// Headers
#include "subtitles_provider.h"
#include "ass_file.h"
#include "video_context.h"
#ifdef WIN32
#define CSRIAPI __declspec(dllimport)
#endif
#include "csri/csri.h"


///////////////////
// Link to library
#if __VISUALC__ >= 1200
#pragma comment(lib,"asa.lib")
#endif


/////////////////////////////////////////////////
// Common Subtitles Rendering Interface provider
class CSRISubtitlesProvider : public SubtitlesProvider {
private:
	csri_inst *instance;

public:
	CSRISubtitlesProvider();
	~CSRISubtitlesProvider();

	bool CanRaster() { return true; }

	void LoadSubtitles(AssFile *subs);
	void DrawSubtitles(AegiVideoFrame &dst,double time);
};


///////////
// Factory
class CSRISubtitlesProviderFactory : public SubtitlesProviderFactory {
public:
	SubtitlesProvider *CreateProvider() { return new CSRISubtitlesProvider(); }
	CSRISubtitlesProviderFactory() : SubtitlesProviderFactory(_T("csri")) {}
} registerCSRI;


///////////////
// Constructor
CSRISubtitlesProvider::CSRISubtitlesProvider() {
	instance = NULL;
}


//////////////
// Destructor
CSRISubtitlesProvider::~CSRISubtitlesProvider() {
	if (instance) csri_close(instance);
	instance = NULL;
}


//////////////////
// Load subtitles
void CSRISubtitlesProvider::LoadSubtitles(AssFile *subs) {
	// Close
	if (instance) csri_close(instance);
	instance = NULL;

	// Prepare subtitles
	//wxString subsfilename = VideoContext::Get()->GetTempWorkFile();
	//subs->Save(subsfilename,false,false,_T("UTF-8"));
	std::vector<char> data;
	subs->SaveMemory(data,_T("UTF-8"));
	delete subs;

	// Open
	//instance = csri_open_file(csri_renderer_default(),subsfilename.mb_str(wxConvUTF8),NULL);
	instance = csri_open_mem(csri_renderer_default(),&data[0],data.size(),NULL);
}


//////////////////
// Draw subtitles
void CSRISubtitlesProvider::DrawSubtitles(AegiVideoFrame &dst,double time) {
	// Check if CSRI loaded properly
	if (!instance) return;

	// Load data into frame
	csri_frame frame;
	for (int i=0;i<4;i++) {
		if (dst.flipped) {
			frame.planes[i] = dst.data[i] + (dst.h-1) * dst.pitch[i];
			frame.strides[i] = -(signed)dst.pitch[i];
		}
		else {
			frame.planes[i] = dst.data[i];
			frame.strides[i] = dst.pitch[i];
		}
	}
	switch (dst.format) {
		case FORMAT_RGB32: frame.pixfmt = CSRI_F_BGR_; break;
		case FORMAT_RGB24: frame.pixfmt = CSRI_F_BGR; break;
		default: frame.pixfmt = CSRI_F_BGR_;
	}

	// Set format
	csri_fmt format;
	format.width = dst.w;
	format.height = dst.h;
	format.pixfmt = frame.pixfmt;
	int error = csri_request_fmt(instance,&format);
	if (error) return;

	// Render
	csri_render(instance,&frame,time);
}
