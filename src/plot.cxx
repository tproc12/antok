
#include<assert.h>
#include<sstream>

#include<cutter.h>
#include<plot.h>

antok::Plot::Plot(std::vector<int>& cutmasks, TH1* hist_template, double* data1, double* data2) :
		_cutmasks(cutmasks),
		_hist_template(hist_template),
		_data1(data1),
		_data2(data2)
{

	assert(hist_template != 0);
	if(_cutmasks.size() == 0) {
		_cutmasks.push_back(0);
	}
	antok::Cutter* cutter = antok::Cutter::instance();
	unsigned int no_cuts = cutter->get_no_cuts();
	std::stringstream sstr;
	bool found_zero = false;
	for(unsigned int i = 0; i < _cutmasks.size(); ++i) {
		if(_cutmasks.at(i) == 0) {
			found_zero = true;
			break;
		}
	}
	if(!found_zero) {
		_cutmasks.push_back(0);
	}
	for(unsigned int i = 0; i < _cutmasks.size(); ++i) {
		int cutmask = _cutmasks.at(i);
		sstr<<hist_template->GetName()<<"_";
		for(int j = no_cuts-1; j >= 0; --j) {
			if((cutmask>>j)&0x1) {
				sstr<<"1";
			} else {
				sstr<<"0";
			}
		}
		TH1* new_hist = dynamic_cast<TH1*>(hist_template->Clone(sstr.str().c_str()));
		assert(new_hist != 0);
		sstr.str("");
		sstr<<hist_template->GetTitle();
		sstr<<" "<<cutter->get_abbreviations(cutmask);
		new_hist->SetTitle(sstr.str().c_str());
		sstr.str("");
		_histograms.push_back(new_hist);
	}

};

void antok::Plot::fill(int cutmask) {

	for(unsigned int i = 0; i < _cutmasks.size(); ++i) {
		if(((~_cutmasks.at(i))&(~cutmask)) == (~_cutmasks.at(i))) {
			assert(_data1 != 0);
			if(_data2 == 0) {
				double data1 = *_data1;
				_histograms.at(i)->Fill(data1);
			} else {
				_histograms.at(i)->Fill(*_data1, *_data2);
			}
		}
	}

}

