local function CheckIsSpecialID( ID )
    local n = tonumber(ID)
    local digit = {}
    for i=1,6 do
        table.insert(digit, 1, n%10)
        n = math.floor(n/10)
    end

    -- 有三个连续的数字
    local same_cnt = 1
    for i=2,6 do
    	if digit[i] == digit[i-1] then
    		same_cnt = same_cnt + 1
    		
	    	if same_cnt == 3 then
	    		return true
	    	end
    	else
    		same_cnt = 1
    	end
    end

    -- ABABAB
    if digit[1] == digit[3] and digit[3] == digit[5] and digit[2] == digit[4] and digit[4] == digit[6] then
    	return true
    end

    -- AABBCC
    if digit[1] == digit[2] and digit[3] == digit[4] and digit[5] == digit[6] then
    	return true
    end

    
    local set = {
    	{0, 1, 2, 2, 1, 0},	-- ABCCBA
    	{0, 1, 2, 0, 1, 2}, -- ABCABC
    	{0, 1, 2, 3, 4, 5}, -- 连续
    }

    for _,t in ipairs(set) do
    	-- AES
    	local issort = true
	    for i=1,6 do
	    	if digit[i] + t[i] ~= digit[1] then
	    		issort = false
	    		break
	    	end
	    end
	    if issort then return true end

	    -- DES
	    local issort = true
	    for i=1,6 do
	    	if digit[i] - t[i] ~= digit[1] then
	    		issort = false
	    		break
	    	end
	    end
	    if issort then return true end
    end
    
    return false
end

local testdata = require("test_id")
for id,v in pairs(testdata) do
	local result = CheckIsSpecialID(id)
	print(id, v, result)
	assert(v==result, id)
end
