package com.capstone.whereigo;

import android.os.Bundle;
import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.fragment.app.FragmentTransaction;

import com.capstone.whereigo.databinding.ActivitySettingBinding;

public class SettingActivity extends AppCompatActivity {
    private static final String TAG = "SettingActivity";

    private ActivitySettingBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        EdgeToEdge.enable(this);
        binding = ActivitySettingBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        binding.toolbar.setNavigationOnClickListener(v -> {
            getSupportFragmentManager().popBackStack();
        });

        ViewCompat.setOnApplyWindowInsetsListener(binding.settingMain, (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        getSupportFragmentManager().beginTransaction()
                .setCustomAnimations(
                        R.anim.enter_anim,       // 새 Fragment 등장
                        R.anim.exit_anim,        // 현재 Fragment 퇴장
                        R.anim.pop_enter_anim,   // 뒤로갈 때 새 Fragment 등장
                        R.anim.pop_exit_anim     // 뒤로갈 때 현재 Fragment 퇴장
                )
                .replace(R.id.setting_list, new SetPreferenceFragment())
                .commit();
    }
}
