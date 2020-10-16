#pragma once

#include <functional>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>

#include <GLFW/glfw3.h>

namespace {

	typedef unsigned int uint;

	namespace TurboPlexer {

		namespace util {

			namespace moreutil {
				inline bool endsWith(const std::string& s, const std::string& suffix) {
					return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
				}
			}

			inline std::vector<std::string> split(const std::string& s, const std::string& delimiter, const bool& removeEmptyEntries = false) {
				std::vector<std::string> tokens;
				for (size_t start = 0, end; start < s.length(); start = end + delimiter.length()) {
					size_t position = s.find(delimiter, start);
					end = position != std::string::npos ? position : s.length();

					std::string token = s.substr(start, end - start);
					if (!removeEmptyEntries || !token.empty())
						tokens.push_back(token);
				}
				if (!removeEmptyEntries && (s.empty() || moreutil::endsWith(s, delimiter)))
					tokens.push_back("");
				return tokens;
			}

		}

		struct Event {
			Event() { std::memset(this, 0, sizeof(Event)); }
			uint codepoint;
			double mousePosition[2];
			bool entered;
			double scrolled[2];
			//
			int button; 
			int action; 
			int mods;
			//
			int key;
			int scancode; 
			int action; 
			int mods;
		};

		class TurboPlexer {

			struct State {
				bool firstClick = true;
				double oldMousePos[2];
				double deltaMouse[2];

				std::unordered_map<std::string, std::vector<const std::function<void(const Event&)>>> delegates[6];

				void KeyCallback(GLFWwindow*, int, int, int, int); //0
				void CharCallback(GLFWwindow*, uint); //1
				void MouseButtonCallback(GLFWwindow*, int, int, int); //2
				void CursorPosCallback(GLFWwindow*, double, double); //3
				void CursorEnterCallback(GLFWwindow*, int); //4
				void ScrollCallback(GLFWwindow*, double, double); //5
			};

			State* currentState = nullptr;
			std::unordered_map<uintptr_t, std::unique_ptr<State>> states;

			static TurboPlexer* instance;

			std::unordered_map<char, int> keymap_glfw;
			std::unordered_map<char, int> keymap_custom;

			TurboPlexer();

			/* binds a window (-> state) */
			void bindState(GLFWwindow*);
			void unbindState(GLFWwindow*);
			void loadState(GLFWwindow*);
			void parseFilter(const std::string&, const std::function<void(const Event&)>);			
			void bindDelegate(const std::function<void(const Event&)>);
		public:
			static TurboPlexer& get(GLFWwindow*);
			void insert(GLFWwindow*, const std::string&, const std::function<void(const Event&)>, const int = 0);
			void remove(const uint);
			void registerPair(const uint, const char);
		};

		TurboPlexer* TurboPlexer::instance = new TurboPlexer();

		inline TurboPlexer::TurboPlexer() {}

		inline void TurboPlexer::bindState(GLFWwindow* _state) {
			std::unique_ptr<State> state = std::make_unique<State>();
			glfwSetWindowUserPointer(_state, state.get());
			glfwSetKeyCallback(_state, [](GLFWwindow* _state, int _key, int _scancode, int _action, int _mods)->void {
				State* state = reinterpret_cast<State*>(glfwGetWindowUserPointer(_state));
				state->KeyCallback(_state, _key, _scancode, _action, _mods);
			});
			glfwSetCharCallback(_state, [](GLFWwindow* _state, uint _codepoint)->void {
				State* state = reinterpret_cast<State*>(glfwGetWindowUserPointer(_state));
				state->CharCallback(_state, _codepoint);
			});
			glfwSetMouseButtonCallback(_state, [](GLFWwindow* _state, int _button, int _action, int _mods)->void {
				State* state = reinterpret_cast<State*>(glfwGetWindowUserPointer(_state));
				state->MouseButtonCallback(_state, _button, _action, _mods);
			});
			glfwSetCursorPosCallback(_state, [](GLFWwindow* _state, double _xpos, double _ypos)->void {
				State* state = reinterpret_cast<State*>(glfwGetWindowUserPointer(_state));
				state->CursorPosCallback(_state, _xpos, _ypos);
			});
			glfwSetCursorEnterCallback(_state, [](GLFWwindow* _state, int _entered)->void {
				State* state = reinterpret_cast<State*>(glfwGetWindowUserPointer(_state));
				state->CursorEnterCallback(_state, _entered);
			});
			glfwSetScrollCallback(_state, [](GLFWwindow* _state, double _xoffset, double _yoffset)->void {
				State* state = reinterpret_cast<State*>(glfwGetWindowUserPointer(_state));
				state->ScrollCallback(_state, _xoffset, _yoffset);
			});
			const uintptr_t id = reinterpret_cast<uintptr_t>(_state);
			states[id] = std::move(state);
		}

		inline void TurboPlexer::unbindState(GLFWwindow* _state) {
			const uintptr_t id = reinterpret_cast<uintptr_t>(_state);
			assert(states.count(id) > 0 && "TurboPlexer::removeState: state doesnt exist.");
			states.erase(id);
			glfwSetWindowUserPointer(_state, nullptr);
			glfwSetKeyCallback(_state, 0);
			glfwSetCharCallback(_state, 0);
			glfwSetMouseButtonCallback(_state, 0);
			glfwSetCursorPosCallback(_state, 0);
			glfwSetCursorEnterCallback(_state, 0);
			glfwSetScrollCallback(_state, 0);
		}

		inline void TurboPlexer::loadState(GLFWwindow* _state) {
			assert(_state != nullptr && "TurboPlexer::loadState: the glfw window handle cant be null");
			const uintptr_t id = reinterpret_cast<uintptr_t>(_state);
			if (!states.count(id)) bindState(_state);
			currentState = states[id].get();
			//do stuff
		}

		/*
			filter syntax:
			event: '[]' - same delegate can be bound to multiple events: '[...][...]...'

			mouse position: [_axis_] - axis: x, y
			mouse buttons: [_button_;_state_] - buttons: 1-8, 'left', 'right', 'middle' - state: 'press', 'release'
			keys: '[_key_;_modifier_,_modifier_,...;_state_]' - valid modifiers: '', 'alt', 'ctrl', 'shift', 'super', 'caps', 'numlock' - special: unicode: {_keycode_;unicode;""}

			key combination: [{_key_;_state_}{_key_;_state_}...;_time_] - keys: needs at least 2 pairs. bind mouse buttons with '$'. time: max time elapsed between strokes.  - state: 'press', 'release', 'repeat'
		*/
		inline void TurboPlexer::parseFilter(const std::string& _filter, const std::function<void(const Event&)> _delegate) {
			assert(!_filter.empty() && "TurboPlexer::parseFilterAndInsert: filter can't be empty");
			const std::vector<std::string> ss = util::split(_filter, "][");
			for (size_t i = 0; i < ss.size(); ++i) {
				//get next substring
				const std::string& s = ss[i];
				//remove '[' or ']'
				const std::string s_r = !i ? s.substr(1) : i == ss.size() - 1 ? s.substr(0, s.size() - 1) : s;
				//check if key combination
				const size_t is_combination = s_r.find_first_of('{', 0);
				if (is_combination == 0) {
					


				} else { //not a combination
					const std::vector<std::string> sd = util::split(s_r, ";");
					switch (sd.size()) {
						case 1: //mouse pos
						{
							const std::string mp = sd[0];
							if (mp.size() == 2)
								bindDelegate(_delegate);
							else {
								if (mp == "x") bindDelegate(_delegate);
								else bindDelegate(_delegate);
							}
						}
						break;
						case 2://mouse button
						{
							//get button
							const std::string& button = sd[0];
							int btn = std::stoi(button);
							assert(btn >= 0 && btn <= 8 && "buttons need to be between [0, 8]");
								
							//get state
							const std::string& state = sd[1];
							int st = 0;
							if (state == "press") {
								st = GLFW_PRESS;
							} else if (state == "release") {
								st = GLFW_RELEASE;
							} else assert(false && "only 'press', 'release' are valid");

							bindDelegate(_delegate);
						}
						break;
						case 3://key
						{
							//get key
							const std::string& key = sd[0];
							const int k = std::stoi(key);

							//'', 'alt', 'ctrl', 'shift', 'super', 'capslock', 'numlock', 'unicode'
							//get modifier
							const std::string& modifier = sd[1];
							const std::vector<std::string> mod_split = util::split(modifier, ",");
							uint mod = 0;
							bool isUnicode = false;
							for (const auto& m : mod_split) {
								if (m.empty()) continue;
								if (m == "alt") {
									mod |= GLFW_MOD_SHIFT;
								} else if (m == "ctrl") {
									mod |= GLFW_MOD_CONTROL;
								} else if (m == "shift") {
									mod |= GLFW_MOD_ALT;
								} else if (m == "super") {
									mod |= GLFW_MOD_SUPER;
								} else if (m == "capslock") {
									mod |= GLFW_MOD_CAPS_LOCK;
								} else if (m == "numlock") {
									mod |= GLFW_MOD_NUM_LOCK;
								} else if (m == "unicode") {
									isUnicode = true;
									break;
								} else assert(false && "invalid modifier");
							}

							//get state
							const std::string& state = sd[2];
							int st = 0;
							if (state == "press") {
								st = GLFW_PRESS;
							} else if (state == "release") {
								st = GLFW_RELEASE;
							} else if (state == "repeat") {
								st = GLFW_REPEAT;
							} else assert(false && "only 'press', 'release', 'repeat' are valid");

							bindDelegate(_delegate);
						}
						break;
					}
				}	
			}
		}

		inline void TurboPlexer::bindDelegate(const std::function<void(const Event& _event)>) {

		}

		inline void TurboPlexer::insert(GLFWwindow* _state, const std::string& _filter, const std::function<void(const Event&)> _delegate, const int _priority) {
			loadState(_state);
			parseFilter(_filter, _delegate);
		}

		inline TurboPlexer& TurboPlexer::get(GLFWwindow* _state) {
			instance->loadState(_state);
			return *instance;
		}

		inline void TurboPlexer::remove(const uint _scancode) {

		}

		inline void TurboPlexer::registerPair(const uint _scancode, const char _character) {

		}

		/*
			_key: The (glfw) keyboard key that was pressed or released.
			_scancode: The system-specific scancode of the key.
			_action: GLFW_PRESS, GLFW_RELEASE or GLFW_REPEAT. Future releases may add more actions.
			_mods: Bit field describing which modifier keys were held down.
		*/
		inline void TurboPlexer::State::KeyCallback(GLFWwindow* _window, int _key, int _scancode, int _action, int _mods) {
			auto state = get(_window).currentState;

			Event e;
			e.key = _key;
			e.scancode = _scancode;
			e.action = _action;
			e.mods = _mods;

			for (const auto& del : state->delegates[0][std::to_string(_key) + std::to_string(_scancode) + std::to_string(_action) + std::to_string(_mods)])
				del(e);
		}

		//unicode callback
		inline void TurboPlexer::State::CharCallback(GLFWwindow* _window, uint _codepoint) {
			auto state = get(_window).currentState;
			Event e;
			e.codepoint = _codepoint;
			for (const auto& del : state->delegates[1][std::to_string(_codepoint)])
				del(e);
		}

		inline void TurboPlexer::State::MouseButtonCallback(GLFWwindow* _window, int _button, int _action, int _mods) {
			auto state = get(_window).currentState;

			Event e;
			e.button = _button;
			e.action = _action;
			e.mods = _mods;

			for (const auto& del : state->delegates[2][std::to_string(_button) + std::to_string(_action) + std::to_string(_mods)])
				del(e);
		}

		inline void TurboPlexer::State::CursorPosCallback(GLFWwindow* _window, double _xpos, double _ypos) {
			auto state = get(_window).currentState;

			//has x moved
			const bool xMoved = std::abs(state->oldMousePos[0] - _xpos) < std::numeric_limits<double>::epsilon();

			//has y moved
			const bool yMoved = std::abs(state->oldMousePos[1] - _ypos) < std::numeric_limits<double>::epsilon();

			Event e;
			e.mousePosition[0] = _xpos;
			e.mousePosition[1] = _ypos;

			if(xMoved)
				for (const auto& del : state->delegates[3]["x"])
					del(e);

			if(yMoved)
				for (const auto& del : state->delegates[3]["y"])
					del(e);
		}

		inline void TurboPlexer::State::CursorEnterCallback(GLFWwindow* _window, int _entered) {
			auto state = get(_window).currentState;

			Event e;
			e.entered = (bool)_entered;

			if (_entered) 
				for (const auto& del : state->delegates[4]["entered"])
					del(e);
			else
				for (const auto& del : state->delegates[4]["left"])
					del(e);
			
		}

		inline void TurboPlexer::State::ScrollCallback(GLFWwindow* _window, double _xoffset, double _yoffset) {
			auto state = get(_window).currentState;

			//has x moved
			const bool xMoved = _xoffset == 0.;

			//has y moved
			const bool yMoved = _yoffset == 0.;

			Event e;
			e.scrolled[0] = _xoffset;
			e.scrolled[1] = _yoffset;

			if (xMoved)
				for (const auto& del : state->delegates[5]["x"])
					del(e);

			if (yMoved)
				for (const auto& del : state->delegates[5]["y"])
					del(e);

		}

	}

	inline uint TurboPlexer(GLFWwindow* _window, const std::string _filter, const std::function<void(const TurboPlexer::Event&)> _delegate, const int _priority = 0) {
		TurboPlexer::TurboPlexer::get(_window).insert(_window, _filter, _delegate, _priority);
	}

	inline bool TurboPlexer_Remove(GLFWwindow* _window, const uint _id) {
		TurboPlexer::TurboPlexer::get(_window).remove(_id);
	}

	inline void TurboPlexer_RegisterCustomEvent(GLFWwindow* _window, const uint _scancode, const char _character) {
		TurboPlexer::TurboPlexer::get(_window).registerPair(_scancode, _character);
	}

}