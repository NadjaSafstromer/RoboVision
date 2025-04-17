function robocup()
    figure('Name', 'RoboCup 11v11 Simulation with Kicking', 'NumberTitle', 'off', 'Color', 'w');

    fieldLength = 9;
    fieldWidth = 6;
    numFrames = 300;
    dt = 0.05;

    blueTeam = init_team(-1);
    yellowTeam = init_team(1);
    ball = [0, 0];
    ballVel = [0.05, 0.02];

    score_blue = 0;
    score_yellow = 0;

    for t = 1:numFrames
        clf;
        hold on;
        axis equal;
        xlim([-fieldLength/2, fieldLength/2]);
        ylim([-fieldWidth/2, fieldWidth/2]);

        % Draw field
        rectangle('Position', [-fieldLength/2, -fieldWidth/2, fieldLength, fieldWidth], ...
                  'FaceColor', [0.1, 0.6, 0.1], 'EdgeColor', 'k', 'LineWidth', 2);
        line([0, 0], [-fieldWidth/2, fieldWidth/2], 'Color', 'w', 'LineStyle', '--', 'LineWidth', 1.5);
        viscircles([0, 0], 0.5, 'Color', 'w', 'LineStyle', ':');

        % Draw goals
        rectangle('Position', [-fieldLength/2-0.1, -0.65, 0.2, 1.3], ...
          'FaceColor', [1 1 1]*0.9, 'EdgeColor', 'k');
        rectangle('Position', [fieldLength/2-0.1, -0.65, 0.2, 1.3], ...
          'FaceColor', [1 1 1]*0.9, 'EdgeColor', 'k');

        % Team movements
        blueTeam = move_team(blueTeam, ball, -1);
        yellowTeam = move_team(yellowTeam, ball, 1);

        % Attacker logic
        blueAttacker = closest_to_ball(blueTeam, ball);
        yellowAttacker = closest_to_ball(yellowTeam, ball);
        [ball, ballVel] = ballKickCheck(ball, ballVel, blueTeam, yellowTeam, blueAttacker, yellowAttacker);

        % Move ball
        ball = ball + ballVel;

        % Goal detection
        if ball(1) < -fieldLength/2 && abs(ball(2)) < 0.65
            score_yellow = score_yellow + 1;
            ball = [0, 0];
            ballVel = [0, 0];
        elseif ball(1) > fieldLength/2 && abs(ball(2)) < 0.65
            score_blue = score_blue + 1;
            ball = [0, 0];
            ballVel = [0, 0];
        end

        % Wall bounce
        r = 0.05;
        if ball(1) - r <= -fieldLength/2 || ball(1) + r >= fieldLength/2
            ballVel(1) = -ballVel(1);
        end
        if ball(2) - r <= -fieldWidth/2 || ball(2) + r >= fieldWidth/2
            ballVel(2) = -ballVel(2);
        end
        ballVel = ballVel * 0.98;

        % Draw robots
        for i = 1:size(blueTeam,1)
            draw_robot(blueTeam(i,:), 'blue');
        end
        for i = 1:size(yellowTeam,1)
            draw_robot(yellowTeam(i,:), 'yellow');
        end

        % Draw ball
        draw_ball(ball);

        % Title
        title(sprintf('Blue: %d   |   Yellow: %d', score_blue, score_yellow), ...
            'FontSize', 14, 'Color', 'k');
        drawnow;
        pause(dt);
    end
end

function team = init_team(side)
    x_base = side * 3;
    team = [
        x_base, 0;
        x_base - 1, -2;
        x_base - 1, -1;
        x_base - 1,  1;
        x_base - 1,  2;
        x_base, -2;
        x_base,  0;
        x_base,  2;
        x_base + 1, -1.5;
        x_base + 1,  0;
        x_base + 1,  1.5;
    ];
end

function team = move_team(team, ball, side)
    for i = 1:size(team,1)
        pos = team(i,:);
        role = get_role(i);
        distance_to_ball = norm(ball - pos);

        % Base speed per role
        if strcmp(role, 'FWD')
            speed = 0.05;
        elseif strcmp(role, 'MID')
            speed = 0.02;
        elseif strcmp(role, 'DEF')
            speed = 0.015;
        else
            speed = 0.02; % GK
        end

        % Role-based targeting
        if strcmp(role, 'FWD')
            target = ball;
        elseif strcmp(role, 'MID')
            zone = [side * 1, pos(2)];
            blend = 0.7;
            target = blend * zone + (1-blend) * ball;
        elseif strcmp(role, 'DEF')
            zone = [side * 2, pos(2)];
            blend = 0.9;
            target = blend * zone + (1-blend) * ball;
        else
            target = [side * 4.2, ball(2)];
        end

        % Speed boost if near ball
        if distance_to_ball < 2.0
            speed = speed * 2;
        end

        new_pos = move_toward(pos, target, speed);
        team(i,:) = avoid_teammates(new_pos, team, i);
    end
end

function newPos = move_toward(pos, target, speed)
    dir = target - pos;
    dist = norm(dir);
    if dist > 0.01
        dir = dir / dist;
        newPos = pos + dir * speed;
    else
        newPos = pos;
    end
end

function pos = avoid_teammates(pos, team, index)
    min_dist = 0.4;
    push_strength = 0.15;
    for i = 1:size(team,1)
        if i == index, continue; end
        diff = pos - team(i,:);
        dist = norm(diff);
        if dist < min_dist && dist > 0
            away = diff / dist;
            pos = pos + away * push_strength;
        end
    end
end

function role = get_role(index)
    if index == 1
        role = 'GK';
    elseif index >= 2 && index <= 5
        role = 'DEF';
    elseif index >= 6 && index <= 8
        role = 'MID';
    else
        role = 'FWD';
    end
end

function draw_robot(pos, color)
    r = 0.09;
    rectangle('Position', [pos(1)-r, pos(2)-r, 2*r, 2*r], ...
              'Curvature', [1 1], ...
              'FaceColor', color, ...
              'EdgeColor', 'k');
end

function draw_ball(pos)
    r = 0.05;
    rectangle('Position', [pos(1)-r, pos(2)-r, 2*r, 2*r], ...
              'Curvature', [1 1], ...
              'FaceColor', 'w', ...
              'EdgeColor', 'k');
end

function idx = closest_to_ball(team, ball)
    dists = vecnorm(team - ball, 2, 2);
    [~, idx] = min(dists);
end

function [ball, ballVel] = ballKickCheck(ball, ballVel, team1, team2, idx1, idx2)
    allPlayers = [team1; team2];
    total = size(allPlayers, 1);

    for i = 1:total
        dist = norm(ball - allPlayers(i,:));
        if dist < 0.12
            if i <= size(team1, 1) && i ~= idx1
                continue;
            elseif i > size(team1, 1) && (i - size(team1, 1)) ~= idx2
                continue;
            end

            if i <= size(team1, 1)
                kicker = team1(i,:);
                teammates = team1;
                goalTarget = [4.5, 0];
            else
                kicker = team2(i - size(team1, 1), :);
                teammates = team2;
                goalTarget = [-4.5, 0];
            end

            % Find teammate to pass to
            % Find opponent team
if i <= size(team1, 1)
    opponents = team2;
else
    opponents = team1;
end

% Check for nearby opponent pressure
underPressure = false;
for j = 1:size(opponents,1)
    if norm(opponents(j,:) - kicker) < 0.6
        underPressure = true;
        break;
    end
end

% Find pass target if not under pressure
minDist = inf;
passTarget = [];
if ~underPressure
    for j = 1:size(teammates,1)
        if norm(teammates(j,:) - kicker) < 2 && norm(teammates(j,:) - kicker) > 0.1
            d = norm(ball - teammates(j,:));
            if d < minDist
                minDist = d;
                passTarget = teammates(j,:);
            end
        end
    end
end

% Decision: pass or shoot
if ~isempty(passTarget)
    dir = passTarget - kicker;
else
    dir = goalTarget - kicker;
end

            dir = dir / norm(dir);
            ballVel = dir * 0.25;
            return;
        end
    end
end