#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

// Predictive Input Buffer
//
// Pomysl: jesli klikniesz za wczesnie (gracz jest jeszcze w powietrzu) i puscisz
// przycisk zanim gracz wyladuje, mod NIE przekazuje zwolnienia przycisku od razu.
// Trzyma klikniecie "wcisniete" do chwili ladowania (skok sie wykona), a potem
// je puszcza. Calosc ograniczona do okna czasowego z ustawien (domyslnie 100 ms).

class $modify(BufferedPlayer, PlayerObject) {
	struct Fields {
		float clock = 0.f;          // wlasny zegar (sekundy)
		float pressTime = 0.f;      // kiedy nacisnieto przycisk
		bool pending = false;       // czekamy z zwolnieniem przycisku
		bool sawGround = false;     // gracz dotknal ziemi, skok powinien sie wykonac
		PlayerButton pendingButton = PlayerButton::Jump;
	};

	static bool bufferActive() {
		return Mod::get()->getSettingValue<bool>("buffer-on") && PlayLayer::get() != nullptr;
	}

	static float windowSeconds() {
		return static_cast<float>(Mod::get()->getSettingValue<int64_t>("window-ms")) / 1000.f;
	}

	void update(float dt) {
		PlayerObject::update(dt);

		auto f = m_fields.self();
		f->clock += dt;

		if (!f->pending) return;

		if (m_isOnGround) {
			// Wyladowal: gra zaraz wykona skok, bo przycisk jest nadal "trzymany".
			f->sawGround = true;
		}

		bool jumped = f->sawGround && !m_isOnGround;
		bool expired = f->clock >= f->pressTime + windowSeconds();

		if (jumped || expired) {
			f->pending = false;
			f->sawGround = false;
			// Wywolanie oryginalu (z pominieciem naszego hooka).
			PlayerObject::releaseButton(f->pendingButton);
		}
	}

	bool pushButton(PlayerButton button) {
		auto f = m_fields.self();
		if (button == PlayerButton::Jump) {
			f->pressTime = f->clock;
			if (f->pending) {
				// Nowe klikniecie w trakcie oczekiwania: zwolnij stare, zacznij od nowa.
				f->pending = false;
				f->sawGround = false;
				PlayerObject::releaseButton(button);
			}
		}
		return PlayerObject::pushButton(button);
	}

	bool releaseButton(PlayerButton button) {
		if (button == PlayerButton::Jump && bufferActive()) {
			auto f = m_fields.self();
			float held = f->clock - f->pressTime;
			// Krotkie klikniecie w powietrzu: odloz zwolnienie.
			if (!m_isOnGround && held < windowSeconds() && !f->pending) {
				f->pending = true;
				f->sawGround = false;
				f->pendingButton = button;
				return true;
			}
		}
		return PlayerObject::releaseButton(button);
	}

	void resetObject() {
		PlayerObject::resetObject();
		auto f = m_fields.self();
		f->pending = false;
		f->sawGround = false;
		f->clock = 0.f;
		f->pressTime = 0.f;
	}
};
