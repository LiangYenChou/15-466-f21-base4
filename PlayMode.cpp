#include "PlayMode.hpp"


#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <fstream>
#include "data_path.hpp"
#include "Sound.hpp"

#if defined(_WIN32)
#include <filesystem>
#else
#include <dirent.h>
#endif


Load< std::map<std::string, Sound::Sample>> sound_samples(LoadTagDefault, []() -> std::map<std::string, Sound::Sample> const* {
		auto map_p = new std::map<std::string, Sound::Sample>();
		std::string base_dir = data_path("musics");

#if defined(_WIN32)
	for (const auto& entry : std::filesystem::directory_iterator(base_dir)) {
		std::string path_string = entry.path().string();
		std::string file_name = entry.path().filename().string();
#else
	struct dirent *entry;
	DIR *dp;

	dp = opendir(&base_dir[0]);
	if (dp == nullptr) {
		std::cout<<"Cannot open "<<base_dir<<"\n";
		throw std::runtime_error("Cannot open dir");
	}
	while ((entry = readdir(dp))) {
		std::string path_string = base_dir + "/" + std::string(entry->d_name);
		std::string file_name = std::string(entry->d_name);
#endif
			size_t start = 0;
			size_t end = file_name.find(".opus");

			if(end != std::string::npos) {
				map_p->emplace(file_name.substr(start, end), Sound::Sample(path_string));
			}
		}
		return map_p;
	});

Load< Sound::Sample > load_typing_effect(LoadTagDefault, []() -> Sound::Sample const* {
		return new Sound::Sample(data_path("musics/typing.opus"));
	});
Load< Sound::Sample > load_bgm(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("musics/bgm.opus"));
	});
Load< Sound::Sample > load_game_end(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("musics/game-end.opus"));
	});

PlayMode::PlayMode() {
	FT_Library library;
	FT_Error error_lib = FT_Init_FreeType(&library);
	if (error_lib) {
		std::cout << "Init library failed." << std::endl;
	}

	std::string desc_font_path = data_path("Hey October.ttf");
	FT_Error error_1 = FT_New_Face(library, &(desc_font_path[0]), 0, &desc_face);

	if (error_1) {
		std::cout<<"Error code: "<<error_1<<std::endl;
		throw std::runtime_error("Cannot open font file");
	}

	FT_Error error_2 = FT_New_Face(library, &(desc_font_path[0]), 0, &option_face);
	if (error_2) {
		std::cout<<"Error code: "<<error_2<<std::endl;
		throw std::runtime_error("Cannot open font file");
	}

	if (!desc_face || !option_face) {
		throw std::runtime_error("Wrong font!");
	}

	scene_sen = new Sentence(desc_face, 720 / LINE_CNT); // hard code height of each line
	for(int i=0; i<5; i++) {
		option_sens.emplace_back(new Sentence(option_face, 720 / LINE_CNT));
	}

	load_text_scenes();
	curr_choice = 0;
	curr_scene = 0; // 1
	bgm_loop = Sound::loop(*load_bgm, 0.15f);
}

PlayMode::~PlayMode() {

}

bool PlayMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {
	// Reference https://github.com/Dmcdominic/15-466-f20-game4/blob/menu-mode/MenuMode.cpp
	if (!text_scenes[curr_scene].choice_descriptions.empty() &&
			text_scenes[curr_scene].description.size() == text_scenes[curr_scene].visible_desc.size()) {
		if (evt.type == SDL_KEYDOWN) {
			if (evt.key.keysym.sym == SDLK_DOWN) {
				curr_choice += 1;
				curr_choice %= text_scenes[curr_scene].choice_descriptions.size();
				return true;
			}
			else if (evt.key.keysym.sym == SDLK_UP) {
				curr_choice -= 1;
				if (curr_choice < 0)
					curr_choice += (int)text_scenes[curr_scene].choice_descriptions.size();
				return true;
			}
			else if (evt.key.keysym.sym == SDLK_SPACE) {
				text_scenes[curr_scene].elapsed = 0.0f;//reset
				text_scenes[curr_scene].visible_desc.clear();//reset
				std::map<int, bool>::iterator it;
				for (it = text_scenes[curr_scene].played.begin(); it != text_scenes[curr_scene].played.end(); it++) {
					it->second = false;
				}
				if (have_cd && text_scenes[curr_scene].next_scene[curr_choice] == 10) {
					text_scenes[curr_scene].next_scene[curr_choice] = 11;
				}
				if (have_cd && text_scenes[curr_scene].next_scene[curr_choice] == 6) {
					text_scenes[curr_scene].next_scene[curr_choice] = 12;
				}
				if (curr_scene == 9 && text_scenes[curr_scene].next_scene[curr_choice] == 9) {
					if (text_scenes[curr_scene].description.find(pc_password) != std::string::npos) {
						pc_unlock = true;
						text_scenes[curr_scene].next_scene[curr_choice] = 13;
					}
					else {
						size_t pos = text_scenes[curr_scene].description.find("EB");
						text_scenes[curr_scene].description.replace(pos, pc_password.length(), pc_original);
					}
				}
				if (curr_scene == 8 && text_scenes[curr_scene].next_scene[curr_choice] == 8) {
					if (text_scenes[curr_scene].description.find(door_password) != std::string::npos) {
						text_scenes[curr_scene].next_scene[curr_choice] = 15;
					}
					else {
						size_t pos = text_scenes[curr_scene].description.find("PIN:") + 5;
						text_scenes[curr_scene].description.replace(pos, door_password.length(), door_original);
					}
				}
				if (pc_unlock && text_scenes[curr_scene].next_scene[curr_choice] == 9) {
					text_scenes[curr_scene].next_scene[curr_choice] = 13;
				}
				if (pc_unlock && text_scenes[curr_scene].next_scene[curr_choice] == 7) {
					text_scenes[curr_scene].next_scene[curr_choice] = 14;
				}
				if (text_scenes[curr_scene].next_scene[curr_choice] == 11) {
					bgm_loop->stop();
				}
				if (curr_scene == 11) {
					bgm_loop = Sound::loop(*load_bgm, 0.15f);
				}
				curr_scene = text_scenes[curr_scene].next_scene[curr_choice];
				curr_choice = 0;
				if (curr_scene == 6) {
					have_cd = true;
				}
				if (curr_scene == 15) {
					bgm_loop->stop();
					Sound::loop(*load_game_end, 0.2f);
				}
			}
			else if (curr_scene == 9 && evt.key.keysym.sym >= 97 && evt.key.keysym.sym <= 122) {
				Sound::play((*sound_samples).at("key_single_press"), 0.4f);
				if (text_scenes[curr_scene].description.find("_") != std::string::npos) {
					text_scenes[curr_scene].description[text_scenes[curr_scene].description.find("_")] = (char)(evt.key.keysym.sym - 97 + 'A');
				}
			}
			else if (curr_scene == 0 && evt.key.keysym.sym >= 48 && evt.key.keysym.sym <= 57) { // curr_scene == 8
				Sound::play((*sound_samples).at("door_pin_single_press"), 0.4f);
				if (text_scenes[curr_scene].description.find("-") != std::string::npos) {
					text_scenes[curr_scene].description[text_scenes[curr_scene].description.find("-")] = (char)(evt.key.keysym.sym - 48 + '0');
				}
			}
			else if (curr_scene == 11 && evt.key.keysym.sym == SDLK_r) {
				Sound::play((*sound_samples).at("code"), 1.0f);
				if (text_scenes[curr_scene].description.find("-") != std::string::npos) {
					text_scenes[curr_scene].description[text_scenes[curr_scene].description.find("-")] = (char)(evt.key.keysym.sym - 48 + '0');
				}
			}
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	{
		// update scene description word
		// check if needs to play sound
		auto cur_len = (uint32_t)text_scenes[curr_scene].visible_desc.size();
		if (!typing_sample && cur_len == 0) {
			typing_sample = Sound::loop(*load_typing_effect, 0.3f);
		}

		if (typing_sample && cur_len == (uint32_t)text_scenes[curr_scene].description.size()) {
			typing_sample->stop();
			typing_sample = nullptr;
		}

		text_scenes[curr_scene].elapsed += elapsed;

		int char_size = std::max((int)(text_scenes[curr_scene].elapsed / pop_char_interval),
						   (int)text_scenes[curr_scene].visible_desc.size());

		text_scenes[curr_scene].visible_desc = text_scenes[curr_scene].description.substr(0, char_size);
		if (text_scenes[curr_scene].sounds.find((int)text_scenes[curr_scene].visible_desc.size()) != text_scenes[curr_scene].sounds.end()) {
			if (!text_scenes[curr_scene].played[(int)text_scenes[curr_scene].visible_desc.size()]) {
				Sound::play((*sound_samples).at(text_scenes[curr_scene].sounds[(int)text_scenes[curr_scene].visible_desc.size()]));
				text_scenes[curr_scene].played[(int)text_scenes[curr_scene].visible_desc.size()] = true;
			}
		}
	}

	{
		// last scene
		if(text_scenes[curr_scene].description.size() == text_scenes[curr_scene].visible_desc.size() &&
				curr_scene == 15) {
			// last scene, show white background and "THE END"
			ending_elapsed += elapsed;
		}
	}
}

void PlayMode::draw(glm::uvec2 const &window_size) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if(ending_elapsed > 0.0f) {
		float val = std::min(ending_elapsed / transition_last_sec, 1.0f);

		glClearColor(val, val, val, 1.0f);
	} else {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	}
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.


	// Reference: https://github.com/Dmcdominic/15-466-f20-game4/blob/menu-mode/MenuMode.cpp
	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);

		scene_sen->ClearText();
		float scene_y_anchor = 1.0f - 2.0f / (float)LINE_CNT;
		scene_sen->SetText(&(text_scenes[curr_scene].visible_desc[0]), 4000,
		                   scene_desc_color,glm::vec2(-0.96f, scene_y_anchor));

		for (auto s : option_sens) {
			s->ClearText();
		}

		int lines_of_desc = (int)std::count(text_scenes[curr_scene].description.begin(),
								 text_scenes[curr_scene].description.end(), '\n') + 1;
		float option_y_anchor = 1.0f - (2.0f / (float)LINE_CNT) * (float)(lines_of_desc + 1);
		for (uint32_t i = 0; i < text_scenes[curr_scene].choice_descriptions.size() && i < option_sens.size(); i++) {
			glm::u8vec4 color = (int)i == curr_choice ? option_select_color : option_unselect_color;

			option_sens[i]->SetText(&(text_scenes[curr_scene].choice_descriptions[i][0]),
			                        3000, color, glm::vec2(-0.96f, option_y_anchor));
			option_y_anchor -= (2.0f / (float)LINE_CNT) / 2.0f;
		}

		scene_sen->Draw(window_size);

		if(text_scenes[curr_scene].description.size() == text_scenes[curr_scene].visible_desc.size()) {
			// only draw until all scene desc appear
			for(auto s: option_sens) {
				s->Draw(window_size);
			}
		}
	}
}


void PlayMode::load_text_scenes() {
	// From https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
	std::string base_dir = data_path("texts");


#if defined(_WIN32)
	for (const auto& entry : std::filesystem::directory_iterator(base_dir)) {
		std::string txt_path = entry.path().string();
#else
	struct dirent *entry;
	DIR *dp;

	dp = opendir(&base_dir[0]);
	if (dp == nullptr) {
		std::cout<<"Cannot open "<<base_dir<<"\n";
		throw std::runtime_error("Cannot open dir");
	}
	while ((entry = readdir(dp))) {
		std::string txt_path = base_dir + "/" + std::string(entry->d_name);
#endif
		std::ifstream f(txt_path);
		std::string line;
		if (!f.is_open()) {
			std::cout << "Unable to open file " << txt_path << std::endl;
			continue;
		}
		if (!std::getline(f, line)) {
			std::cout << txt_path << " is an empty file! Skipped." << std::endl;
			continue;
		}
		int id = std::stoi(line);
		std::string description = "";

		while (std::getline(f, line)) {
			if (line.rfind("##", 0) == 0)
				break;
			if (line.rfind("&&", 0) == 0)
				break;
			description.append(line.append("\n"));
		}
		description = description.substr(0, description.size() - 1);
		TextScene ts = { id, description };
		while (line.rfind("##", 0) == 0) {
			ts.next_scene.push_back(std::stoi(line.substr(2)));
			std::string choice_des = "";
			while (std::getline(f, line)) {
				if (line.rfind("##", 0) == 0)
					break;
				if (line.rfind("&&", 0) == 0)
					break;
				choice_des.append(line.append("\n"));
			}
			choice_des = choice_des.substr(0, choice_des.size() - 1);
			ts.choice_descriptions.push_back(choice_des);
		}
		while (line.rfind("&&", 0) == 0) {
			int start_pos = std::stoi(line.substr(2));
			if (!std::getline(f, line)) {
				std::cout << "Music not specified!" << std::endl;
				break;
			}
			ts.sounds[start_pos] = line;
			ts.played[start_pos] = false;
			std::getline(f, line);
		}
		text_scenes[id] = ts;
		text_scenes[id].elapsed = 0.0f; // init timer
		f.close();
	}

#if defined(_WIN32)
#else
	closedir(dp);
#endif
}