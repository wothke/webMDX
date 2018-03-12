/*
 mdx_adapter.js: Adapts MDX/PMD backend to generic WebAudio/ScriptProcessor player.
 
 version 1.0
 
 	Copyright (C) 2018 Juergen Wothke

 LICENSE
 
 This library is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or (at
 your option) any later version. This library is distributed in the hope
 that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/
MDXBackendAdapter = (function(){ var $this = function () {
		$this.base.call(this, backend_mdx.Module, 2);
		this._manualSetupComplete= true;
		this._undefined;
		this._currentPath;
		this._currentFile;
		this._chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
		if (!backend_mdx.Module.notReady) {
			// in sync scenario the "onRuntimeInitialized" has already fired before execution gets here,
			// i.e. it has to be called explicitly here (in async scenario "onRuntimeInitialized" will trigger
			// the call directly)
			this.doOnAdapterReady();
		}				
	}; 
	// sample buffer contains 2-byte integer sample data (i.e. 
	// must be rescaled) of 2 interleaved channels
	extend(EmsHEAP16BackendAdapter, $this, {
		doOnAdapterReady: function() {
			// called when runtime is ready (e.g. asynchronously when WASM is loaded)
			// if FS needed to be setup of would be done here..
		},
		getAudioBuffer: function() {
			var ptr=  this.Module.ccall('emu_get_audio_buffer', 'number');			
			// make it a this.Module.HEAP16 pointer
			return ptr >> 1;	// 2 x 16 bit samples			
		},
		getAudioBufferLength: function() {
			var len= this.Module.ccall('emu_get_audio_buffer_length', 'number');
			return len;
		},
		computeAudioSamples: function() {
			return this.Module.ccall('emu_compute_audio_samples', 'number');
		},
		getMaxPlaybackPosition: function() { 
			return this.Module.ccall('emu_get_max_position', 'number');
		},
		getPlaybackPosition: function() {
			return this.Module.ccall('emu_get_current_position', 'number');
		},
		seekPlaybackPosition: function(pos) {
			this.Module.ccall('emu_seek_position', 'number', ['number'], [pos]);
		},		

		// map external files info the FS so that they can be accessed using regular FILE*  based operations
		mapUrl: function(filename) {
			// used transform the "internal filename" to a valid URL
			var uri= this.mapFs2Uri(filename);
			var p= uri.indexOf("@");	// cut off "basePath" for "outside" files
			if (p >= 0) {
				uri= uri.substring(p+1);
			}			
			return uri;
		},
		mapInternalFilename: function(overridePath, basePath, filename) {
			filename= this.mapUri2Fs("@" + filename);	// treat all songs as "from outside"

			return ((overridePath)?overridePath:basePath) + filename;
		},
		
		getPathAndFilename: function(filename) {
			var sp = filename.split('/');
			var fn = sp[sp.length-1];					
			var path= filename.substring(0, filename.lastIndexOf("/"));	
			if (path.lenght) path= path+"/";
			
			return [path, fn];
		},
		mapBackendFilename: function (name) {
			// "name" comes from the C++ side 
			var input= this.Module.Pointer_stringify(name);
			return input;
		},
		registerFileData: function(pathFilenameArray, data) {
			return this.registerEmscriptenFileData(pathFilenameArray, data);
		},
		loadMusicData: function(sampleRate, path, filename, data, options) {
			var buf = this.Module._malloc(data.length);
			this.Module.HEAPU8.set(data, buf);
			var ret = this.Module.ccall('emu_load_file', 'number', ['string', 'number', 'number'], [path+"/"+filename, buf, data.length]);
			this.Module._free(buf);

			if (ret == 0) {
				this.playerSampleRate = this.Module.ccall('emu_get_sample_rate', 'number');
				this.resetSampleRate(sampleRate, this.playerSampleRate);
				this._currentPath= path;
				this._currentFile= filename;
			} else {
				this._currentPath= this._undefined;
				this._currentFile= this._undefined;
			}
			return ret;			
		},
		evalTrackOptions: function(options) {
			if (typeof options.timeout != 'undefined') {
				ScriptNodePlayer.getInstance().setPlaybackTimeout(options.timeout*1000);
			} else {
				ScriptNodePlayer.getInstance().setPlaybackTimeout(-1);	// reset last songs setting
			}
			var id= (options && options.track) ? options.track : -1;	// by default do not set track		
			var boostVolume= (options && options.boostVolume) ? options.boostVolume : 0;		
			return this.Module.ccall('emu_set_subsong', 'number', ['number', 'number'], [id, boostVolume]);	// not used here..
		},				
		teardown: function() {
			this.Module.ccall('emu_teardown', 'number');	// just in case
		},
		// base64 decoding util
		findChar: function(str, c) {
			for (var i= 0; i<str.length; i++) {
				if (str.charAt(i) == c) {
					return i;
				}
			}
			return -1;
		},
		alphanumeric: function(inputtxt) {
			var letterNumber = /^[0-9a-zA-Z]+$/;
			return inputtxt.match(letterNumber);
		},
		is_base64: function(c) {
		  return (this.alphanumeric(""+c) || (c == '+') || (c == '/'));
		}, 
		base64Decode: function(encoded) {
			var in_len= encoded.length;
			var i= j= in_= 0;
			var arr4= new Array(4);
			var arr3= new Array(3);
			var ret= "";
			var carry=-1;

			while (in_len-- && ( encoded.charAt(in_) != '=') && this.is_base64(encoded.charAt(in_))) {
				arr4[i++]= encoded.charAt(in_); in_++;
				if (i ==4) {
					for (i = 0; i <4; i++) {
						arr4[i] = this.findChar(this._chars, arr4[i]);
					}
					arr3[0] = ( arr4[0] << 2       ) + ((arr4[1] & 0x30) >> 4);
					arr3[1] = ((arr4[1] & 0xf) << 4) + ((arr4[2] & 0x3c) >> 2);
					arr3[2] = ((arr4[2] & 0x3) << 6) +   arr4[3];

					for (i = 0; (i < 3); i++) {
						var val= arr3[i];
						
						if (carry > -1) {	// only allow 16bit max
							val= (carry << 8) + val;
							carry= -1;
							ret += String.fromCharCode(val)	// UNICODE
							
						} else if (val > 127) {	// treat as unicode
							carry= val;
						} else {
							ret += String.fromCharCode(val);	// ASCII
						}
					}
					i = 0;
				}
			}
			if (i) {
				for (j = 0; j < i; j++) {
					arr4[j] = this.findChar(this._chars, arr4[j]);
				}
				arr3[0] = (arr4[0] << 2) + ((arr4[1] & 0x30) >> 4);
				arr3[1] = ((arr4[1] & 0xf) << 4) + ((arr4[2] & 0x3c) >> 2);

				for (j = 0; (j < i - 1); j++) { 
					var val= arr3[j];
					
					if (carry > -1) {	// only allow 16bit max
						val= (carry << 8) + val;
						carry= -1;
						ret += String.fromCharCode(val)	// UNICODE
						
					} else if (val > 127) {	// treat as unicode
						carry= val;
					} else {
						ret += String.fromCharCode(val);	// ASCII
					}
				}
			}
			return ret;
		},		
		getSongInfoMeta: function() {
			return {title: String,
					artist: String, 
					};
		},
		
		updateSongInfo: function(filename, result) {
			var numAttr= 2;
			var ret = this.Module.ccall('emu_get_track_info', 'number');

			// the automatic string creation fucks up the UNICODE chars beyond 
			// recognition.. base64	wrapping is used to handle the strings properly	
			var array = this.Module.HEAP32.subarray(ret>>2, (ret>>2)+numAttr);
			result.title= this.base64Decode(this.Module.Pointer_stringify(array[0]));
			result.artist= this.base64Decode(this.Module.Pointer_stringify(array[1]));
		}
	});	return $this; })();